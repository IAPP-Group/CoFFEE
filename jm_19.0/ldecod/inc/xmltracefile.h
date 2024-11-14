#ifndef _XML_TRACE_H_
#define _XML_TRACE_H_

#define XML_TRACE_VERSION	"1.4"

#include <stdio.h>

void xml_set_trace_filename(char* filename);
char* xml_get_trace_filename();

void xml_set_log_level(int level);
int xml_get_log_level();

int xml_open_trace_file();
void xml_close_trace_file();

void xml_write_start_element(char* name);
void xml_write_end_element();

void xml_write_string_attribute(char* name, char* value);
void xml_write_int_attribute(char* name, int value);

void xml_write_text(char* value);
void xml_write_int(int value);
void xml_write_comment(char* comment);

int xml_check_and_write_end_element(char* name);

int xml_gen_trace_file();

int xml_output_frame_order(int);

void xml_trace_setIsIntra(int intra);

//binary functions
void binary_write_start_element(char* name);
void binary_write_end_element();
int binary_check_and_write_end_element(char* name, unsigned char end_tag);

#endif
