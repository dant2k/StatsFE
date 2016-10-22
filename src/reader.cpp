#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <time.h>
#include <Windows.h>

#include <vector>

#include <assert.h>

#include <limits.h>
extern char const* input;

#pragma pack(push, 1)
struct file_header
{
    int rec_type;
    char prod_name[60];
    int layout_code;
    int nominal_case_size;
    int compression;
    int weight_index;
    int ncases;
    double bias;
    char creation_date[9];
    char creation_time[8];
    char file_label[64];
    char padding[3];
};

struct variable_record_header
{
    int rec_type;
    int var_type;
    int has_var_label;
    int n_missing_value;
    int print_format;
    int write_format;
    char name[8];
};
struct variable_record
{
    variable_record_header header;
    std::string proper_name;
    int label_len;
    char* labels;
    double* missing_values;
    variable_record* parent_string;
    size_t detected_label;
    char* str_buffer;
    std::vector<double> values;
    std::vector<char> str_values;
};

struct value_label
{
    int rec_type;
    int label_count;

    struct value_label_entry
    {
        char value[8];
        char label_len;
        char label[1];
    };
    std::vector<value_label_entry*> labels;
    std::vector<int> var_indeces;
};

#define int32 int
#define flt64 double

struct int_record
{
    /* Header. */
    int32               rec_type;
    int32               subtype;
    int32               size;
    int32               count;

    /* Data. */
    int32               version_major;
    int32               version_minor;
    int32               version_revision;
    int32               machine_code;
    int32               floating_point_rep;
    int32               compression_code;
    int32               endianness;
    int32               character_code;
};

struct double_record
{
    /* Header. */
    int32               rec_type;
    int32               subtype;
    int32               size;
    int32               count;

    /* Data. */
    flt64               sysmis;
    flt64               highest;
    flt64               lowest;
};

struct var_display_record
{
    /* Header. */
    int32               rec_type;
    int32               subtype;
    int32               size;
    int32               count;

    /* Repeated count times. */
    int32               measure;
    int32               width;           /* Not always present. */
    int32               alignment;
};
#pragma pack(pop)


void main()
{
    FILE* fh = fopen("C:\\users\\soal\\desktop\\test_private.sav", "rb");
    fseek(fh, 0, SEEK_END);
    long file_len = ftell(fh);
    fseek(fh, 0, SEEK_SET);
    unsigned char* raw_file = (unsigned char*)malloc(file_len);
    fread(raw_file, file_len, 1, fh);
    fclose(fh);

    unsigned char* cur_file = raw_file;
    unsigned char* end_file = raw_file + file_len;
    file_header header;
    memcpy(&header, cur_file, sizeof(header));
    cur_file += sizeof(header);

    std::vector<variable_record*> variables;
    variable_record* last_real_record = 0;

    std::vector<value_label> value_labels;    

    for (;;)
    {
        int rec_type;
        memcpy(&rec_type, cur_file, 4);
        
        if (rec_type == 2)
        {
            // var record
            variable_record_header var_header;
            memcpy(&var_header, cur_file, sizeof(variable_record_header));
            cur_file += sizeof(variable_record_header);

            if (var_header.var_type == -1)
            {
                variable_record* r = new variable_record();
                memcpy(r, &var_header, sizeof(variable_record_header));
                r->parent_string = last_real_record;
                variables.push_back(r);
                continue;
            }

            variable_record* record = 0;
            if (var_header.has_var_label)
            {
                int label_len;
                memcpy(&label_len, cur_file, 4);
                cur_file += 4;

                label_len = (label_len + 3) & ~3;

                record = (variable_record*)malloc(sizeof(variable_record) + label_len);
                new (record) variable_record();
                memcpy(record, &var_header, sizeof(variable_record_header));
                record->label_len = label_len;
                
                memcpy(record + 1, cur_file, label_len);
                cur_file += label_len;

                record->labels = (char*)(record + 1);
            }
            else
            {
                record = new variable_record();
                record->label_len = 0;
                memcpy(record, &var_header, sizeof(variable_record_header));
            }
            if (var_header.n_missing_value)
                assert(0);

            if (var_header.print_format != var_header.write_format)
                assert(0);

            //printf("VAR: %c%c%c%c%c%c%c%c", 
            //    record->header.name[0], record->header.name[1], record->header.name[2], record->header.name[3], 
            //    record->header.name[4], record->header.name[5], record->header.name[6], record->header.name[7]);
            //if (record->header.has_var_label)
            //    printf(" [%s]", record->labels);
            //printf("\n");
            if (record->header.var_type)
                record->str_buffer = new char[record->header.var_type + 1];

            record->parent_string = 0;
            variables.push_back(record);
            last_real_record = record;
        }
        else if (rec_type == 3)
        {
            cur_file += 4;
            // value labels;
            value_label label;
            
            memcpy(&label.label_count, cur_file, 4);
            cur_file += 4;

            for (int i = 0; i < label.label_count; i++)
            {
                char label_len = cur_file[8];
                
                int label_len_with_padding = label_len;
                label_len_with_padding++;
                label_len_with_padding = (label_len_with_padding + 7) & ~7;
                
                value_label::value_label_entry* entry = (value_label::value_label_entry*)malloc(8 + label_len_with_padding);
                memcpy(entry->value, cur_file, 8);
                entry->label_len = label_len;
                cur_file += 9;

                memcpy(entry->label, cur_file, label_len_with_padding - 1);
                cur_file += label_len_with_padding - 1;

                label.labels.push_back(entry);
            }

            value_labels.push_back(label);
        }
        else if (rec_type == 4)
        {
            cur_file += 4;
            int var_count;
            
            memcpy(&var_count, cur_file, 4);
            cur_file += 4;

            for (int i = 0; i < var_count; i++)
            {
                int var;
                
                memcpy(&var, cur_file, 4);
                cur_file += 4;
                value_labels.back().var_indeces.push_back(var);

                variables[var]->detected_label = value_labels.size() - 1;
            }
        }
        else if (rec_type == 7)
        {
            cur_file += 4;
            int sub_type;
            memcpy(&sub_type, cur_file, 4);
            cur_file += 4;

            int size, count;
            memcpy(&size, cur_file, 4);
            memcpy(&count, cur_file + 4, 4);
            cur_file += 8;

            if (sub_type == 13)
            {
                unsigned char* kvp = cur_file;
                unsigned char* kvp_end = cur_file + count;
                for (; kvp != kvp_end; )
                {
                    unsigned char* split = (unsigned char*)strchr((char*)kvp, '=');
                    char check_name[8];
                    memset(check_name, ' ', 8);
                    memcpy(check_name, kvp, split - kvp);

                    unsigned char* end = kvp;
                    while (*end != 9 && end != kvp_end)
                        end++;


                    // find the variable
                    for (size_t v = 0; v < variables.size(); v++)
                    {
                        if (variables[v]->parent_string)
                            continue;

                        if (memcmp(variables[v]->header.name, check_name, 8) == 0)
                        {
                            variables[v]->proper_name.append((char*)split+1, end - (split + 1));
                            break;
                        }
                    }

                    kvp = end;
                    if (end != kvp_end)
                        kvp++;
                }
            }
            int bytes_to_seek = size * count;
            cur_file += bytes_to_seek;
        }
        else if (rec_type == 999)
        {
            cur_file += 4;
            // we should be at the data records
            cur_file += 4; // filler

            unsigned char command_stream[8];
            memcpy(command_stream, cur_file, 8);
            cur_file += 8;
            
            size_t current_variable = 0;

            do
            {
                for (int i = 0; i < 8; i++)
                {
                    variable_record* current_record = variables[current_variable];
                    current_variable++;
                    if (current_variable == variables.size())
                        current_variable = 0;

                    if (command_stream[i] == 0)
                        break; // done
                    if (command_stream[i] == 252)
                        goto done;
                    if (command_stream[i] == 253)
                    {
                        if (current_record->header.var_type == 0)
                        {
                            double value;
                            memcpy(&value, cur_file, 8);
                            cur_file += 8;
                            current_record->values.push_back(value);
                        }
                        else
                        {
                            variable_record* string_record = current_record;
                            if (current_record->parent_string)
                                string_record = current_record->parent_string;

                            int current_offset = string_record->str_values.size() % string_record->header.var_type;
                            int remn = string_record->header.var_type - current_offset;
                            if (remn > 8)
                                remn = 8;

                            int padded_len = (string_record->header.var_type + 7) & ~7;
                            for (int s = 0; s < remn; s++)
                            {
                                string_record->str_values.push_back(cur_file[s]);
                            }

                            cur_file += (remn + 7) & ~7;
                        }
                    }
                    else if (command_stream[i] == 254)
                    {
                        variable_record* string_record = current_record;
                        if (current_record->parent_string)
                            string_record = current_record->parent_string;

                        if (string_record->header.var_type == 0)
                            assert(0);

                        // find how many of this set of 8 go to the variable.
                        int current_offset = string_record->str_values.size() % string_record->header.var_type;
                        int remn = string_record->header.var_type - current_offset;
                        if (remn > 8)
                            remn = 8;
                        for (int s = 0; s < remn; s++)
                            string_record->str_values.push_back(' ');
                    }
                    else if (command_stream[i] == 255)
                    {
                        if (current_record->header.var_type == 0)
                        {
                            current_record->values.push_back(DBL_MAX);
                        }
                        else
                        {
                            for (int s = 0; s < current_record->header.var_type; s++)
                            {
                                current_record->str_values.push_back('-');
                            }
                        }
                    }
                    else
                    {
                        double value = (double)command_stream[i] - header.bias;

                        if (current_record->header.var_type == 0)
                        {
                            current_record->values.push_back(value);
                        }
                        else
                        {
                            if (value != 0)
                                assert(0);

                            {
                                for (int s = 0; s < current_record->header.var_type; s++)
                                {
                                    current_record->str_values.push_back(0);
                                }
                            }
                        }
                    }
                }

                // read the next command stream
                if (cur_file + 8 > end_file)
                    goto done;
                memcpy(command_stream, cur_file, 8);
                cur_file += 8;
            } while (true);
        }
    }
done:

    for (size_t i = 0; i < variables.size(); i++)
    {
        if (variables[i]->parent_string)
            continue;
        if (variables[i]->proper_name.size() == 0)
        {
            char fmt_name[9];
            memcpy(fmt_name, variables[i]->header.name, 8);
            fmt_name[8] = 0;
            size_t trim = 7;
            while (fmt_name[trim] == ' ')
            {
                fmt_name[trim] = 0;
                trim--;
            }
            variables[i]->proper_name = fmt_name;
        }
    }


    //
    // Parse the input string for the usevariables.
    //
    char const* usevars = strstr(input, "USEVARIABLES");
    char const* arevars = strstr(usevars, "ARE");

    arevars += 3;
    while (*arevars == ' ' ||
        *arevars == '\r' ||
        *arevars == '\n')
        arevars++;

    char const* endvars = strchr(arevars, ';');

    // parse out the variable names.
    std::vector<std::string> used_variables;
    for (; arevars < endvars; )
    {
        char const* next_space = strchr(arevars, ' ');
        if (next_space == 0)
            next_space = endvars;
        char const* next_cr = strchr(arevars, '\r');
        if (next_cr == 0)
            next_cr = endvars;
        char const* next_lf = strchr(arevars, '\n');
        if (next_lf == 0)
            next_lf = endvars;

        char const* use_end  = next_space;
        if (next_cr < use_end)
            use_end = next_cr;
        if (next_lf < use_end)
            use_end = next_lf;

        std::string v;
        v.append(arevars, use_end - arevars);
        used_variables.push_back(v);

        arevars = use_end;
        while (*arevars == ' ' ||
            *arevars == '\r' ||
            *arevars == '\n')
            arevars++;
    }

    // look up the variables and see how many are in our dataset.
    std::vector<size_t> ref_variables;
    for (size_t i = 0; i < used_variables.size(); i++)
    {
        for (size_t v = 0; v < variables.size(); v++)
        {
            variable_record* record = variables[v];
            if (record->parent_string)
                continue;

            if (_stricmp(record->proper_name.c_str(), used_variables[i].c_str()) == 0)
            {
                ref_variables.push_back(v);
                break;
            }
        }
    }

    // write out a csv of everything to verify we got the data right
    FILE* out = fopen("c:\\users\\soal\\desktop\\reader_mplus_input.csv", "wb");
    
    for (size_t i = 0; i < ref_variables.size(); i++)
    {
        variable_record* record = variables[ref_variables[i]];

        //value_label& l = value_labels[record->detected_label];
        fprintf(out, record->proper_name.c_str());
        if (i < ref_variables.size() - 1)
            fputc(',', out);
    }
    fputc('\r', out);
    fputc('\n', out);

    size_t current = 0;
    for (;;)
    {
        for (size_t i = 0; i < ref_variables.size(); i++)
        {
            variable_record* record = variables[ref_variables[i]];

            if (record->header.var_type == 0)
            {
                if (current < record->values.size())
                {
                    int format_type = (record->header.print_format & 0xff0000) >> 16;
                    if (format_type == 5) // float
                    {
                        if (record->values[current] == DBL_MAX)
                            fprintf(out, " ");
                        else
                        {
                            char blah[128];
                            sprintf(blah, "%.15g", record->values[current]);
                            if (blah[0] == '0' && blah[1] == '.')
                                fprintf(out, blah + 1); // cut the leading 0
                            else if (blah[0] == '-' && blah[1] == '0' && blah[2] == '.')
                            {
                                fputc('-', out);
                                fprintf(out, blah + 2);
                            }
                            else
                                fprintf(out, blah);
                        }
                    }
                    else if (format_type == 23) // ADATE
                    {
                        __int64 seconds_to_1601 = 574905600;

                        if (record->values[current] == DBL_MAX)
                            fprintf(out, " ");
                        else
                        {
                            __int64 filetime = (__int64)(record->values[current]) - seconds_to_1601;
                            // now from seconds to 100nanoseconds
                            filetime *= 10000000;

                            FILETIME win_time;
                            win_time.dwHighDateTime = filetime >> 32;
                            win_time.dwLowDateTime = filetime & 0xffffffff;

                            SYSTEMTIME systime;
                            FileTimeToSystemTime(&win_time, &systime);
                            

                            fprintf(out, "%d/%d/%d", systime.wMonth, systime.wDay, systime.wYear);
                        }
                    }
                    else
                        assert(0);
                }
                else
                    fprintf(out, " ");
            }
            else
            {
                if (current * record->header.var_type < record->str_values.size())
                {
                    char* c = &record->str_values[current * record->header.var_type];
                
                    size_t missing_check = 0;
                    for (; missing_check != record->header.var_type; missing_check++)
                    {
                        if (c[missing_check] != '-')
                            break;
                    }
                    if (missing_check == record->header.var_type)
                    {
                        // missing string var.
                        fprintf(out, " ");
                    }
                    else
                    {
                        memcpy(record->str_buffer, c, record->header.var_type);
                        record->str_buffer[record->header.var_type] = 0;
                        size_t s = record->header.var_type - 1;
                        while (record->str_buffer[s] == ' ' && s >= 1)
                        {
                            record->str_buffer[s] = 0;
                            s--;
                        }
                        if (strchr(record->str_buffer, ',') ||
                            strchr(record->str_buffer, '\"'))
                        {
                            fputc('\"', out);

                            for (char* s = record->str_buffer; *s; s++)
                            {
                                if (*s == '\"')
                                    fputc('\"', out);
                                fputc(*s, out);
                            }

                            fputc('\"', out);
                        }
                        else
                            fprintf(out, "%s", record->str_buffer);
                    }
                }
            }

            if (i < ref_variables.size() - 1)
                fputc(',', out);
        }
        fputc('\r', out);
        fputc('\n', out);
        current++;
        if (current == header.ncases)
            break;
    }
    fclose(out);

    std::string output;
    output.append("TITLE:\nTest Input Generator\n");
    output.append("DATA: FILE IS \"c:\\users\\soal\\desktop\\reader_mplus_input.csv\";\n");
    output.append("VARIABLE: NAMES ARE \n");

    size_t line_len = 0;
    for (size_t i = 0; i < ref_variables.size(); i++)
    {
        output.append(variables[ref_variables[i]]->proper_name);
        output.append(" ");
        line_len += variables[ref_variables[i]]->proper_name.size();
        if (line_len > 60)
        {
            output.append("\n");
            line_len = 0;
        }
    }
    output.append(";\n");
    output.append(input);

    FILE* inp_file = fopen("c:\\users\\soal\\desktop\\reader_mplus.inp", "wb");
    fwrite(output.c_str(), output.size(), 1, inp_file);
    fclose(inp_file);

    // run mplus
    system("c:\\mplus\\mplus.exe c:\\users\\soal\\desktop\\reader_mplus.inp");

    FILE* out_file = fopen("c:\\users\\soal\\desktop\\reader_mplus.out", "rb");

    //size_t current_variable = 0;

    //for (size_t i = 0; i < data.size(); i++)
    //{
    //    if (variables[current_variable]->header.var_type == 0)
    //    {
    //        variables[current_variable]->values.push_back(data[i]);
    //    }
    //    else
    //    {
    //        char str[8];
    //        memcpy(str, &data[i], 8);
    //        printf("string var\n");
    //    }
    //    current_variable++;
    //    if (current_variable == variables.size())
    //        current_variable = 0;
    //}
    //fclose(fh);
}


char const* input = ""
"USEVARIABLES ARE \n"
"risk1 risk2 risk3 \n"
"ap1 ap2 ap3 \n"
"tht1 tht2 tht3 \n"
"pos1 pos2 pos3 \n"
"prob1 prob2 prob3 \n"
"age1 sex ;\n"
""
"MISSING ARE all (-99) ;\n"
"ANALYSIS:   TYPE=MISSING H1;\n"
"            ITERATIONS=10000;\n"
""
"MODEL:\n"
"    pos1 on risk1 ap1 tht1 age1 sex;\n"
"    ap1 on risk1 age1 ;\n"
"    tht1 on risk1 age1 ;\n"
"    pos2 on pos1 risk1 ap1 tht1 risk2 ap2 tht2 age1 sex;\n"
"    ap2 on ap1 risk1 risk2 age1 ; \n"
"    tht2 on tht1 risk1 risk2 age1 ;\n"
"    pos3 on pos2 risk2 ap2 tht2 risk3 ap3 tht3 age1 sex;\n"
"    ap3 on ap2 risk2 risk3 age1 ;\n"
"    tht3 on tht2 risk2 risk3 age1 ;\n"
"    pos1 on risk1 ap1 tht1 age1 sex;\n"
"    ap1 on risk1 age1 ;\n"
"    tht1 on risk1 age1 ;\n"
"    pos2 on pos1 risk1 ap1 tht1 risk2 ap2 tht2 age1 sex;\n"
"    ap2 on ap1 risk1 risk2 age1 ; \n"
"    tht2 on tht1 risk1 risk2 age1 ;\n"
"    pos3 on pos2 risk2 ap2 tht2 risk3 ap3 tht3 age1 sex;\n"
"    ap3 on ap2 risk2 risk3 age1 ;\n"
"    tht3 on tht2 risk2 risk3 age1 ;\n"
"    prob1 on risk1 ap1 tht1 age1 sex;\n"
"    ap1 on risk1 age1 ;\n"
"    tht1 on risk1 age1 ;\n"
"    prob2 on prob1 risk1 ap1 tht1 risk2 ap2 tht2 age1 sex;\n"
"    ap2 on ap1 risk1 risk2 age1 ; \n"
"    tht2 on tht1 risk1 risk2 age1 ;\n"
"    prob3 on prob2 risk2 ap2 tht2 risk3 ap3 tht3 age1 sex;\n"
"    ap3 on ap2 risk2 risk3 age1 ;\n"
"    tht3 on tht2 risk2 risk3 age1 ;\n"
"    prob1 on risk1 ap1 tht1 age1 sex;\n"
"    ap1 on risk1 age1 ;\n"
"    tht1 on risk1 age1 ;\n"
"    prob2 on prob1 risk1 ap1 tht1 risk2 ap2 tht2 age1 sex;\n"
"    ap2 on ap1 risk1 risk2 age1 ; \n"
"    tht2 on tht1 risk1 risk2 age1 ;\n"
"    prob3 on prob2 risk2 ap2 tht2 risk3 ap3 tht3 age1 sex;\n"
"    ap3 on ap2 risk2 risk3 age1 ;\n"
"    tht3 on tht2 risk2 risk3 age1 ;\n"
"risk1 with risk2;\n"
"risk2 with risk3;\n"
"risk3 with risk1;\n"
"risk1 with age1;\n"
"risk1 with sex;\n"
"MODEL INDIRECT:\n"
"   pos2 IND risk1;\n"
"   pos3 IND risk2;\n"
"   prob2 IND risk1;\n"
"   prob3 IND risk2;\n"
""
"OUTPUT:"
"standardized sampstat;\n";