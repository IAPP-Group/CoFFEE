#include "tracehelper.h"
#include "xmltracefile.h"
#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"


int iCurr_frame_id = 0;
char* current_frame_name = "0";
int previous_frame_id = 0;
int maxValue = -9999;
int minValue = 9999;
int iCurr_gop_number = -1;
int iDisplayNumberOffset = 0;
int iCurr_mb_type = 0;
FILE *fptr;

int iCurr_NAL_number = -1;
NALU_t *curr_NAL, *dpb_NAL, *dpc_NAL;

extern FILE* fh;

/*Used to correlate a specific DCT value to an array element of the histogram*/
typedef struct {
    int dct_coeff;
    int histogram_array_index;
} pair_int_int ;

typedef struct {
    int *array;
    size_t used;
    size_t size;
} Array;

void initArray(Array *a, size_t initialSize) {
    a->array = malloc(initialSize * sizeof(int));
    a->used = 0;
    a->size = initialSize;
}

void insertArray(Array *a, int element) {
    // a->used is the number of used entries, because a->array[a->used++] updates a->used only *after* the array has been accessed.
    // Therefore a->used can go up to a->size
    if (a->used == a->size) {
        a->size *= 2;
        a->array = realloc(a->array, a->size * sizeof(int));
    }
    a->array[a->used++] = element;
}

void freeArray(Array *a) {
    free(a->array);
    a->array = NULL;
    a->used = a->size = 0;
}

typedef struct {
    pair_int_int *array;
    size_t used;
    size_t size;
} Map;

void initMap(Map *a, size_t initialSize) {
    a->array = malloc(initialSize * sizeof(pair_int_int));
    a->used = 0;
    a->size = initialSize;
}

void insertMap(Map *a, pair_int_int element) {
    // a->used is the number of used entries, because a->array[a->used++] updates a->used only *after* the array has been accessed.
    // Therefore a->used can go up to a->size
    if (a->used == a->size) {
        a->size *= 2;
        a->array = realloc(a->array, a->size * sizeof(pair_int_int));
    }
    a->array[a->used++] = element;
}

void printMap(Map* a){
    printf("%c", '\n');
    for(int map_idx = 0; map_idx < a->used; map_idx++){
        printf("%d", a->array[map_idx].dct_coeff);
        printf("%c", ':');
        printf("%c", ' ');
        printf("%d", a->array[map_idx].histogram_array_index);
        printf("%c", ',');
        printf("%c", '\n');

    }
}

char * toArray(int number)
{
    int n = log10(number) + 1;
    int i;
    char *numberArray = calloc(n, sizeof(char));
    for (i = n-1; i >= 0; --i, number /= 10)
    {
        numberArray[i] = (number % 10) + '0';
    }
    return numberArray;
}

void freeMap(Map *a) {
    free(a->array);
    a->array = NULL;
    a->used = a->size = 0;
}


Array histogramArray;
Map dct_coeff_to_index;

Array histogramCrArray;
Map dct_coeff_to_index_cr;

Array histogramCbArray;
Map dct_coeff_to_index_cb;



int getNewFrameID()
{
	iCurr_frame_id++;
	return iCurr_frame_id - 1;
}

void incrementGOP()
{
	iCurr_gop_number++;
}

int getGOPNumber()
{
	return iCurr_gop_number;
}

void setDisplayNumberOffset(int value)
{
	iDisplayNumberOffset = value;
}

int getDisplayNumberOffset()
{
	return iDisplayNumberOffset;
}

void incrementNAL()
{
	iCurr_NAL_number++;
}

void setNAL(NALU_t *nal)
{
	incrementNAL();
	curr_NAL = AllocNALU(MAX_CODED_FRAME_SIZE); //allocate memory for a NALU (nalucommon.h)
	curr_NAL->nal_unit_type = nal->nal_unit_type;
	curr_NAL->len = nal->len;
}

void setDPBNAL(NALU_t *nal)
{
	incrementNAL();
	dpb_NAL = AllocNALU(MAX_CODED_FRAME_SIZE);
	dpb_NAL->nal_unit_type = nal->nal_unit_type;
	dpb_NAL->len = nal->len;
}

void setDPCNAL(NALU_t *nal)
{
	incrementNAL();
	dpc_NAL = AllocNALU(MAX_CODED_FRAME_SIZE);
	dpc_NAL->nal_unit_type = nal->nal_unit_type;
	dpc_NAL->len = nal->len;
}

void clearNALInfo()
{
	FreeNALU(curr_NAL);
	curr_NAL = NULL;
	FreeNALU(dpb_NAL);
	dpb_NAL = NULL;
	FreeNALU(dpc_NAL);
	dpc_NAL = NULL;
}

void getMbTypeName_I_Slice(int mb_type, Macroblock* currMB, char* typestring, char* predmodstring, int corr)
{

	strcpy(predmodstring, "BLOCK_TYPE_I");

	switch(mb_type - corr)
	{
		case 0:
			if(currMB->luma_transform_size_8x8_flag == 0)
				strcpy(typestring, "I_4x4");
			else
				strcpy(typestring, "I_8x8");
			break;
        //MB Partition Prediction Mode, Intra 16x16 Pred Mode, Coded Block Pattern Chroma Coded Block Pattern Luma
		case 1: strcpy(typestring, "I_16x16_0_0_0"); break;
		case 2: strcpy(typestring, "I_16x16_1_0_0"); break;
		case 3: strcpy(typestring, "I_16x16_2_0_0"); break;
		case 4: strcpy(typestring, "I_16x16_3_0_0"); break;
		case 5: strcpy(typestring, "I_16x16_0_1_0"); break;
		case 6: strcpy(typestring, "I_16x16_1_1_0"); break;
		case 7: strcpy(typestring, "I_16x16_2_1_0"); break;
		case 8: strcpy(typestring, "I_16x16_3_1_0"); break;
		case 9: strcpy(typestring, "I_16x16_0_2_0"); break;
		case 10: strcpy(typestring, "I_16x16_1_2_0"); break;
		case 11: strcpy(typestring, "I_16x16_2_2_0"); break;
		case 12: strcpy(typestring, "I_16x16_3_2_0"); break;
		case 13: strcpy(typestring, "I_16x16_0_0_1"); break;
		case 14: strcpy(typestring, "I_16x16_1_0_1"); break;
		case 15: strcpy(typestring, "I_16x16_2_0_1"); break;
		case 16: strcpy(typestring, "I_16x16_3_0_1"); break;
		case 17: strcpy(typestring, "I_16x16_0_1_1"); break;
		case 18: strcpy(typestring, "I_16x16_1_1_1"); break;
		case 19: strcpy(typestring, "I_16x16_2_1_1"); break;
		case 20: strcpy(typestring, "I_16x16_3_1_1"); break;
		case 21: strcpy(typestring, "I_16x16_0_2_1"); break;
		case 22: strcpy(typestring, "I_16x16_1_2_1"); break;
		case 23: strcpy(typestring, "I_16x16_2_2_1"); break;
		case 24: strcpy(typestring, "I_16x16_3_2_1"); break;
		case 25: strcpy(typestring, "I_PCM"); break;
		default: strcpy(typestring, "UNKNOWN"); break;
	}

}

void getMbTypeName_SI_Slice(int mb_type, Macroblock* currMB, char* typestring, char* predmodstring, int corr)
{

	strcpy(predmodstring, "BLOCK_TYPE_SI");

	if(mb_type == 0) strcpy(typestring, "SI");
	else getMbTypeName_I_Slice(mb_type, currMB, typestring, predmodstring, 1);

}

void getMbTypeName_P_SP_Slice(int mb_type, Macroblock* currMB, char* typestring, char* predmodstring, int corr)
{

	if(mb_type != 0) mb_type--;

	strcpy(predmodstring, "BLOCK_TYPE_P");

	if(mb_type <= 4)
	{
		switch(mb_type)
		{
			case 0: strcpy(typestring, "P_L0_16x16"); break; //the samples of mb are predicted with one luma mb partition of size 16x16 luma samples and associated chroma samples
			case 1: strcpy(typestring, "P_L0_L0_16x8"); break; //samples of mb predicted using two luma partition 16x8 and associated chroma samples
			case 2: strcpy(typestring, "P_L0_L0_8x16"); break; //equals
			case 3: strcpy(typestring, "P_8x8"); break; //addtional syntax element (sub_mb_type)
			case 4: strcpy(typestring, "P_8x8ref0"); break; 
		}
	}
	else getMbTypeName_I_Slice(mb_type, currMB, typestring, predmodstring, 5);

	if(currMB->skip_flag == 1)
		strcpy(typestring, "P_SKIP");
}

void getMbTypeName_B_Slice(int mb_type, Macroblock* currMB, char* typestring, char* predmodstring, int corr)
{

	strcpy(predmodstring, "BLOCK_TYPE_B");

	if(mb_type <= 22)
	{
		switch(mb_type)
		{
			case 0: strcpy(typestring, "B_Direct_16x16"); break;  //p.106 ITU-T Rec.H.264
			case 1: strcpy(typestring, "B_L0_16x16"); break;
			case 2: strcpy(typestring, "B_L1_16x16"); break;
			case 3: strcpy(typestring, "B_Bi_16x16"); break;
			case 4: strcpy(typestring, "B_L0_L0_16x8"); break;
			case 5: strcpy(typestring, "B_L0_L0_8x16"); break;
			case 6: strcpy(typestring, "B_L1_L1_16x8"); break;
			case 7: strcpy(typestring, "B_L1_L1_8x16"); break;
			case 8: strcpy(typestring, "B_L0_L1_16x8"); break;
			case 9: strcpy(typestring, "B_L0_L1_8x16"); break;
			case 10: strcpy(typestring, "B_L1_L0_16x8"); break;
			case 11: strcpy(typestring, "B_L1_L0_8x16"); break;
			case 12: strcpy(typestring, "B_L0_Bi_16x8"); break;
			case 13: strcpy(typestring, "B_L0_Bi_8x16"); break;
			case 14: strcpy(typestring, "B_L1_Bi_16x8"); break;
			case 15: strcpy(typestring, "B_L1_Bi_8x16"); break;
			case 16: strcpy(typestring, "B_Bi_L0_16x8"); break;
			case 17: strcpy(typestring, "B_Bi_L0_8x16"); break;
			case 18: strcpy(typestring, "B_Bi_L1_16x8"); break;
			case 19: strcpy(typestring, "B_Bi_L1_8x16"); break;
			case 20: strcpy(typestring, "B_Bi_Bi_16x8"); break;
			case 21: strcpy(typestring, "B_Bi_Bi_8x16"); break;
			case 22: strcpy(typestring, "B_8x8"); break;
		}
	}
	else getMbTypeName_I_Slice(mb_type, currMB, typestring, predmodstring, 23);

	if(currMB->skip_flag == 1)
		strcpy(typestring, "B_SKIP");
}

void getSubMbTypeName_P_Slice(int submb_type, char* typestring)
{
	switch(submb_type)
	{
		case 0: strcpy(typestring, "P_L0_8x8"); break;
		case 1: strcpy(typestring, "P_L0_8x4"); break;
		case 2: strcpy(typestring, "P_L0_4x8"); break;
		case 3: strcpy(typestring, "P_L0_4x4"); break;
		default: strcpy(typestring, "UNKNOWN"); break;
	}
}

void getSubMbTypeName_B_Slice(int submb_type, char* typestring)
{
	switch(submb_type)
	{
		case 0: strcpy(typestring, "B_Direct_8x8"); break;
		case 1: strcpy(typestring, "B_L0_8x8"); break;
		case 2: strcpy(typestring, "B_L1_8x8"); break;
		case 3: strcpy(typestring, "B_Bi_8x8"); break;
		case 4: strcpy(typestring, "B_L0_8x4"); break;
		case 5: strcpy(typestring, "B_L0_4x8"); break;
		case 6: strcpy(typestring, "B_L1_8x4"); break;
		case 7: strcpy(typestring, "B_L1_4x8"); break;
		case 8: strcpy(typestring, "B_Bi_8x4"); break;
		case 9: strcpy(typestring, "B_Bi_4x8"); break;
		case 10: strcpy(typestring, "B_L0_4x4"); break;
		case 11: strcpy(typestring, "B_L1_4x4"); break;
		case 12: strcpy(typestring, "B_Bi_4x4"); break;
		default: strcpy(typestring, "UNKNOWN"); break;
	}
}

//MV for P_8x8
//void addMVInfoToTrace(Macroblock *currMB)
//{
//	int j0, i0, j, i, list;
//	int kk, step_h, step_v;
//	char typestring[256];
//
//	if(currMB->mb_type == P8x8)
//	{
//		//Loop through every submacroblock
//		for(j0 = 0; j0 < 4; j0 += 2)	//vertical
//		{
//			for(i0 = 0; i0 < 4; i0 += 2)	//horizontal
//			{
//				kk = 2 * (j0 >> 1) + (i0 >> 1);
//				xml_write_start_element("SubMacroBlock");
//					xml_write_int_attribute("num", kk);
//					xml_write_start_element("Type");
//						xml_write_int(currMB->b8submbtype[kk]);
//					xml_write_end_element();
//					xml_write_start_element("TypeString");
//						if(currMB->p_Slice->slice_type == B_SLICE)
//							getSubMbTypeName_B_Slice(currMB->b8submbtype[kk], typestring);
//						else
//							getSubMbTypeName_P_Slice(currMB->b8submbtype[kk], typestring);
//						xml_write_text(typestring);
//					xml_write_end_element();
//
//					if(xml_get_log_level() >= 3)
//					{
//						//Loop through every block inside the submacroblock to get the motion vectors
//						step_h = BLOCK_STEP [currMB->b8mode[kk]][0];
//						step_v = BLOCK_STEP [currMB->b8mode[kk]][1];
//						if(currMB->b8mode[kk] != 0) //Has forward vector
//						{
//							for (j = j0; j < j0 + 2; j += step_v)
//							{
//								for (i = i0; i < i0 + 2; i += step_h)
//								{
//									//Loop through LIST0 and LIST1
//									for (list = LIST_0; list <= LIST_1; list++)
//									{
//										if ((currMB->b8pdir[kk]== list || currMB->b8pdir[kk]== BI_PRED) && (currMB->b8mode[kk] !=0))//has forward vector
//										{
//											xml_write_start_element("MotionVector");
//												xml_write_int_attribute("list", list);
//												xml_write_start_element("RefIdx");
//													xml_write_int(currMB->p_Vid->dec_picture->motion.ref_id[list][currMB->block_y+j0][currMB->block_x+i0]);
//												xml_write_end_element();
//												xml_write_start_element("Difference");
//													xml_write_start_element("X");
//														xml_write_int(currMB->mvd[list][j][i][0]);
//													xml_write_end_element();
//													xml_write_start_element("Y");
//														xml_write_int(currMB->mvd[list][j][i][1]);
//													xml_write_end_element();
//												xml_write_end_element();
//												xml_write_start_element("Absolute");
//													xml_write_start_element("X");
//														xml_write_int(currMB->p_Vid->dec_picture->motion.mv[list][currMB->block_y + j][currMB->block_x + i][0]);
//													xml_write_end_element();
//													xml_write_start_element("Y");
//														xml_write_int(currMB->p_Vid->dec_picture->motion.mv[list][currMB->block_y + j][currMB->block_x + i][1]);
//													xml_write_end_element();
//												xml_write_end_element();
//											xml_write_end_element();
//										}
//									}
//								}
//							}
//						}
//					}
//				xml_write_end_element();
//			}
//		}
//	}
//}

/*file pointer must be opened*/
void saveHistogram(FILE* file_pointer, Array* array, Map* map){
    fprintf(file_pointer,"%c",'{');
    for(int idx_M = 0; idx_M<(*map).used; idx_M++){
        fprintf(file_pointer,"%c",'"');
        fprintf(file_pointer,"%d",(*map).array[idx_M].dct_coeff);
        fprintf(file_pointer,"%c",'"');
        fprintf(file_pointer,"%c",':');
        fprintf(file_pointer,"%c",' ');
        fprintf(file_pointer,"%d",(*array).array[(*map).array[idx_M].histogram_array_index]);
        if(idx_M!=(*map).used - 1){
            fprintf(file_pointer,"%c",',');
        }
        fprintf(file_pointer,"%c",'\n');
    }
    fprintf(file_pointer,"%c",'}');
}

void computeHistogramToJson(Macroblock* currMB, Slice* currSlice, int isIntraFrame){
    if(isIntraFrame==0){
        return;
    }
    //initArray(&histogramArray,100);
    //initMap(&dct_coeff_to_index,100);

    if(previous_frame_id!=iCurr_frame_id){
        /*I'm writing a new frame*/
        freeArray(&histogramArray);
        initArray(&histogramArray,100);

        freeMap(&dct_coeff_to_index);
        initMap(&dct_coeff_to_index,100);

        freeArray(&histogramCbArray);
        initArray(&histogramCbArray,100);

        freeMap(&dct_coeff_to_index_cb);
        initMap(&dct_coeff_to_index_cb,100);

        freeArray(&histogramCrArray);
        initArray(&histogramCrArray,100);

        freeMap(&dct_coeff_to_index_cr);
        initMap(&dct_coeff_to_index_cr,100);


        previous_frame_id = iCurr_frame_id;
        current_frame_name = toArray(iCurr_frame_id);


    }
    int pl, i, j;

    const char* name = "dct_hist_luma_frame_";
    const char* cr_name = "dct_hist_cb_frame_";
    const char* cb_name = "dct_hist_cr_frame_";

    


    const char* ext = ".json";


    char* name_with_extension;

    name_with_extension = malloc(strlen(name)+strlen(current_frame_name)+strlen(ext)+1);
    strcpy(name_with_extension, name);
    strcat(name_with_extension, current_frame_name);
    strcat(name_with_extension, ext);

    char* name_with_extension_cb;
    name_with_extension_cb = malloc(strlen(cb_name)+strlen(current_frame_name)+strlen(ext)+1);
    strcpy(name_with_extension_cb, cb_name);
    strcat(name_with_extension_cb, current_frame_name);
    strcat(name_with_extension_cb, ext);

    char* name_with_extension_cr;
    name_with_extension_cr = malloc(strlen(cr_name)+strlen(current_frame_name)+strlen(ext)+1);
    strcpy(name_with_extension_cr, cr_name);
    strcat(name_with_extension_cr, current_frame_name);
    strcat(name_with_extension_cr, ext);
    

    if(currMB->luma_transform_size_8x8_flag)
    {
        //Luma

        if (!fptr) perror("fopen");
        for(i = 0; i < 16; i++)
        {
            //printf("%c",'H');
            for(j = 0; j < 16; j++)
            {

                int found = 0;

                for(int idx_M = 0; idx_M<dct_coeff_to_index.used; idx_M++){
                    if(dct_coeff_to_index.array[idx_M].dct_coeff==(&currSlice->mb_rres[0][0])[i][j]){
                        histogramArray.array[dct_coeff_to_index.array[idx_M].histogram_array_index]++;
                        found = 1;
                    }
                }
                if(found==0){
                    insertArray(&histogramArray,1);
                    pair_int_int* p = malloc(sizeof(pair_int_int));
                    p->histogram_array_index = (int)histogramArray.used-1;
                    p->dct_coeff = (&currSlice->mb_rres[0][0])[i][j];
                    insertMap(&dct_coeff_to_index,*p);
                }


            }
        }


       /* fptr = fopen(name_with_extension,"w");
        saveHistogram(fptr, &histogramArray, &dct_coeff_to_index);
        fclose(fptr);*/


        //Chroma

        for(pl = 1; pl <= 2; pl++)
        {

            for( i = 0; i < 8; i++)
            {
                for(j = 0; j < 8; j++)
                {

                    if(pl==1){

                        //CB histogram
                        int found = 0;

                        for(int idx_M = 0; idx_M<dct_coeff_to_index_cb.used; idx_M++){
                            if(dct_coeff_to_index_cb.array[idx_M].dct_coeff==currSlice->cof[pl][i][j]){
                                histogramCbArray.array[dct_coeff_to_index_cb.array[idx_M].histogram_array_index]++;                             
                               
                                found = 1;
                            }
                        }

                        if(found==0){
                            insertArray(&histogramCbArray,1);
                            pair_int_int* p = malloc(sizeof(pair_int_int));
                            p->histogram_array_index = (int)histogramCbArray.used-1;
                            p->dct_coeff = currSlice->cof[pl][i][j];
                            insertMap(&dct_coeff_to_index_cb,*p);
                        }

                        
                        

                    }else{

                        //CR histogram
                        int found = 0;

                        for(int idx_M = 0; idx_M<dct_coeff_to_index_cr.used; idx_M++){
                            if(dct_coeff_to_index_cr.array[idx_M].dct_coeff==currSlice->cof[pl][i][j]){
                                histogramCrArray.array[dct_coeff_to_index_cr.array[idx_M].histogram_array_index]++;
                                found = 1;
                            }
                        }
                        if(found==0){
                            insertArray(&histogramCrArray,1);
                            pair_int_int* p = malloc(sizeof(pair_int_int));
                            p->histogram_array_index = (int)histogramCrArray.used-1;
                            p->dct_coeff = currSlice->cof[pl][i][j];
                            insertMap(&dct_coeff_to_index_cr,*p);
                        }

                        
                    }

                }
            }
        }

       /* fptr = fopen(name_with_extension_cb,"w");
        saveHistogram(fptr, &histogramCbArray, &dct_coeff_to_index_cb);
        fclose(fptr);

        fptr = fopen(name_with_extension_cr,"w");
        saveHistogram(fptr, &histogramCrArray, &dct_coeff_to_index_cr);
        fclose(fptr);*/

    }
    else
    {
        //Luma
        for(i = 0; i < 16; i++)
        {
            for(j = 0; j < 16; j++)
            {
                int found = 0;

                for(int idx_M = 0; idx_M<dct_coeff_to_index.used; idx_M++){
                    if(dct_coeff_to_index.array[idx_M].dct_coeff==currSlice->cof[0][i][j]){
                        histogramArray.array[dct_coeff_to_index.array[idx_M].histogram_array_index]++;
                        /*
                        printf("Found: ");
                        printf("%i",dct_coeff_to_index.array[idx_M].dct_coeff);
                        printf(", Increasing value:");
                        printf("%i",histogramArray.array[dct_coeff_to_index.array[idx_M].histogram_array_index]);
                        printf("\n");
                        */
                        found = 1;
                    }
                }
                if(found==0){
                    insertArray(&histogramArray,1);
                    pair_int_int* p = malloc(sizeof(pair_int_int));
                    p->histogram_array_index = (int)histogramArray.used-1;
                    p->dct_coeff = currSlice->cof[0][i][j];
                    insertMap(&dct_coeff_to_index,*p);
                    /*
                    printf("Found: ");
                    printf("%i",currSlice->cof[0][i][j]);
                    printf(", Creating value: 1");
                    printf("\n");
                     */
                }

            }
        }

       /* fptr = fopen(name_with_extension,"w");
        saveHistogram(fptr, &histogramArray, &dct_coeff_to_index);
        fclose(fptr);*/

        //Chroma
        for(pl = 1; pl <= 2; pl++)
        {
            for( i = 0; i < 8; i++)
            {
                for(j = 0; j < 8; j++)
                {
                    if(pl==1){

                        //CB histogram
                        int found = 0;

                        for(int idx_M = 0; idx_M<dct_coeff_to_index_cb.used; idx_M++){
                            if(dct_coeff_to_index_cb.array[idx_M].dct_coeff==currSlice->cof[pl][i][j]){
                                histogramCbArray.array[dct_coeff_to_index_cb.array[idx_M].histogram_array_index]++;
                                found = 1;
                            }
                        }
                        if(found==0){
                            insertArray(&histogramCbArray,1);
                            pair_int_int* p = malloc(sizeof(pair_int_int));
                            p->histogram_array_index = (int)histogramCbArray.used-1;
                            p->dct_coeff = currSlice->cof[pl][i][j];
                            insertMap(&dct_coeff_to_index_cb,*p);
                        }
                    }else{

                        //CR histogram
                        int found = 0;

                        for(int idx_M = 0; idx_M<dct_coeff_to_index_cr.used; idx_M++){
                            if(dct_coeff_to_index_cr.array[idx_M].dct_coeff==currSlice->cof[pl][i][j]){
                                histogramCrArray.array[dct_coeff_to_index_cr.array[idx_M].histogram_array_index]++;
                                found = 1;
                            }
                        }
                        if(found==0){
                            insertArray(&histogramCrArray,1);
                            pair_int_int* p = malloc(sizeof(pair_int_int));
                            p->histogram_array_index = (int)histogramCrArray.used-1;
                            p->dct_coeff = currSlice->cof[pl][i][j];
                            insertMap(&dct_coeff_to_index_cr,*p);
                        }
                    }

                }
            }
        }
        /*fptr = fopen(name_with_extension_cb,"w");
        saveHistogram(fptr, &histogramCbArray, &dct_coeff_to_index_cb);
        fclose(fptr);

        fptr = fopen(name_with_extension_cr,"w");
        saveHistogram(fptr, &histogramCrArray, &dct_coeff_to_index_cr);
        fclose(fptr);*/
    }
}

void addCoeffsToTrace(Macroblock* currMB, Slice* currSlice)
{

    int pl, i, j;
     
    xml_write_start_element("Coeffs");
    if(currMB->luma_transform_size_8x8_flag)
    {
        //Luma
        xml_write_start_element("Plane");
        xml_write_int_attribute("type", 0);
        
        for(i = 0; i < 16; i++)
        {
            xml_write_start_element("Row");
            for(j = 0; j < 16; j++)
            {
                if(j > 0) xml_write_text(",");
                xml_write_int((&currSlice->mb_rres[0][0])[i][j]);
            }
            xml_write_end_element();
        }
        xml_write_end_element();
       
        ////Chroma

for (pl = 1; pl <= 2; pl++)
{

    xml_write_start_element("Plane");
    xml_write_int_attribute("type", pl);

    for (i = 0; i < 8; i++)
    {
        xml_write_start_element("Row");
        for (j = 0; j < 8; j++)
        {
            if (j > 0) xml_write_text(",");
            xml_write_int(currSlice->cof[pl][i][j]);
        }
        xml_write_end_element();
    }
    xml_write_end_element();
}
    }
    else
    {
        //Luma

        xml_write_start_element("Plane");
        xml_write_int_attribute("type", 0);

        for (i = 0; i < 16; i++)
        {
            xml_write_start_element("Row");
            for (j = 0; j < 16; j++)
            {
                if (j > 0) xml_write_text(",");
                xml_write_int(currSlice->cof[0][i][j]);
            }
            xml_write_end_element();
        }

        xml_write_end_element();

        //Chroma

        for (pl = 1; pl <= 2; pl++)
        {
            xml_write_start_element("Plane");
            xml_write_int_attribute("type", pl);

            for (i = 0; i < 8; i++)
            {
                xml_write_start_element("Row");
                for (j = 0; j < 8; j++)
                {
                    if (j > 0) xml_write_text(",");
                    xml_write_int(currSlice->cof[pl][i][j]);
                }
                xml_write_end_element();
            }
            xml_write_end_element();
        }
    }
    xml_write_end_element();

    return;


}

void binaryAddCoeffsToTrace(Macroblock* currMB, Slice* currSlice)
{

    int pl, i, j;
    unsigned char marker = 0;
    unsigned char plane;
    int zero = 0;
    
    
    int consecutive_zero_counter;
    int num_of_coeff_per_plane;

    int coeff_rle[512];


    if (currMB->luma_transform_size_8x8_flag)
    {
        //Luma
        

        plane = 'L';
        fwrite(&plane, 1, 1, fh);

        consecutive_zero_counter = 0;
        num_of_coeff_per_plane = 0;

        for (i = 0; i < 16; i++)
        {
            for (j = 0; j < 16; j++)
            {
                if ((&currSlice->mb_rres[0][0])[i][j] == 0)
                    consecutive_zero_counter += 1;

                else 
                    if (consecutive_zero_counter > 0) {
                        coeff_rle[num_of_coeff_per_plane++] = 0;
                        coeff_rle[num_of_coeff_per_plane++] = consecutive_zero_counter;
                        coeff_rle[num_of_coeff_per_plane++] = (&currSlice->mb_rres[0][0])[i][j];
                        //fwrite(&(&currSlice->mb_rres[0][0])[i][j], 4, 1, fh);
                        consecutive_zero_counter = 0;
                       
                    }
                    else {
                        coeff_rle[num_of_coeff_per_plane++] = (&currSlice->mb_rres[0][0])[i][j];
                        //fwrite(&(&currSlice->mb_rres[0][0])[i][j], 4, 1, fh);
                        
                    }
  
            }

            
        }

        if (consecutive_zero_counter > 0) {
            coeff_rle[num_of_coeff_per_plane++] = 0;
            coeff_rle[num_of_coeff_per_plane++] = consecutive_zero_counter;

        }

        if (coeff_rle[0] == 0 && coeff_rle[1] == 256) {
            fwrite(&zero, 2, 1, fh);
        }
        else {
            fwrite(&num_of_coeff_per_plane, 2, 1, fh);
            fwrite(coeff_rle, 4, num_of_coeff_per_plane, fh);

        }
        

        binary_check_and_write_end_element("Plane", 0x05);

        ////Chroma


        for (pl = 1; pl <= 2; pl++)
        {
            if (pl == 1)
                plane = 'B';
            else
                plane = 'R';

            fwrite(&plane, 1, 1, fh);

            consecutive_zero_counter = 0;
            num_of_coeff_per_plane = 0;

            for (i = 0; i < 8; i++)
            {
                for (j = 0; j < 8; j++)
                {
                    if (currSlice->cof[pl][i][j] == 0)
                        consecutive_zero_counter += 1;

                    else
                        if (consecutive_zero_counter > 0)
                        {
                            coeff_rle[num_of_coeff_per_plane++] = 0;
                            coeff_rle[num_of_coeff_per_plane++] = consecutive_zero_counter;
                            coeff_rle[num_of_coeff_per_plane++] = currSlice->cof[pl][i][j];
                            consecutive_zero_counter = 0;
                        }
                        else {
                            coeff_rle[num_of_coeff_per_plane++] = currSlice->cof[pl][i][j];
                            
                        }
                            

                }

            }

            if (consecutive_zero_counter > 0) {
                coeff_rle[num_of_coeff_per_plane++] = 0;
                coeff_rle[num_of_coeff_per_plane++] = consecutive_zero_counter;

            }

            if (coeff_rle[0] == 0 && coeff_rle[1] == 64) {
                fwrite(&zero, 2, 1, fh);
            }
            else {
                fwrite(&num_of_coeff_per_plane, 2, 1, fh);
                fwrite(coeff_rle, 4, num_of_coeff_per_plane, fh);
            }
     
            binary_check_and_write_end_element("Plane", 0x05);
        }
    }
    else
    {
        //Luma


        plane = 'L';
        fwrite(&plane, 1, 1, fh);

        consecutive_zero_counter = 0;
        num_of_coeff_per_plane = 0;
        

        for (i = 0; i < 16; i++)
        {
        
            for(j = 0; j < 16; j++)
            {
                if (currSlice->cof[0][i][j] == 0)
                    consecutive_zero_counter += 1;

                else 
                    if (consecutive_zero_counter > 0) {
                        coeff_rle[num_of_coeff_per_plane++] = 0;
                        coeff_rle[num_of_coeff_per_plane++] = consecutive_zero_counter;
                        coeff_rle[num_of_coeff_per_plane++] = currSlice->cof[0][i][j];
                        //fwrite(&currSlice->cof[0][i][j], 4, 1, fh);
                        consecutive_zero_counter = 0; 
  
                    }
                    else {
                        coeff_rle[num_of_coeff_per_plane++] = currSlice->cof[0][i][j];
                        //fwrite(&currSlice->cof[0][i][j], 4, 1, fh);

                    }

            }
            
        }

        if (consecutive_zero_counter > 0) {
            coeff_rle[num_of_coeff_per_plane++] = 0;
            coeff_rle[num_of_coeff_per_plane++] = consecutive_zero_counter;
        }

        if (coeff_rle[0] == 0 && coeff_rle[1] == 256) {
            fwrite(&zero, 2, 1, fh);
        }
        else {

            fwrite(&num_of_coeff_per_plane, 2, 1, fh);
            fwrite(coeff_rle, 4, num_of_coeff_per_plane, fh);
        }

        binary_check_and_write_end_element("Plane", 0x05);

        //Chroma
        for (pl = 1; pl <= 2; pl++)
        {
            //binary_write_start_element("Plane");
            //fwrite(&marker, 1, 1, fh);
         
            if (pl == 1)
                plane = 'B';
            else
                plane = 'R';

            fwrite(&plane, 1, 1, fh);
     
            consecutive_zero_counter = 0;
            num_of_coeff_per_plane = 0;

            for (i = 0; i < 8; i++)
            {
                //fwrite(&marker, 1, 1, fh);

                for(j = 0; j < 8; j++)
                {
                    if (currSlice->cof[pl][i][j] == 0) 
                        consecutive_zero_counter += 1;

                    else
                        if (consecutive_zero_counter > 0)
                        {
                            coeff_rle[num_of_coeff_per_plane++] = 0;
                            coeff_rle[num_of_coeff_per_plane++] = consecutive_zero_counter;
                            coeff_rle[num_of_coeff_per_plane++] = currSlice->cof[pl][i][j];
                            consecutive_zero_counter = 0;
                            
                        }
                        else {
                            coeff_rle[num_of_coeff_per_plane++] = currSlice->cof[pl][i][j];

                        }
                            

                }

              

                



            //    //fwrite(&end_of_row, 1, 1, fh); //end row

            //    //xml_write_end_element();
            }

            if (consecutive_zero_counter > 0) {
                coeff_rle[num_of_coeff_per_plane++] = 0;
                coeff_rle[num_of_coeff_per_plane++] = consecutive_zero_counter;
            }

            if (coeff_rle[0] == 0 && coeff_rle[1] == 64) {
                fwrite(&zero, 2, 1, fh);
            }
            else {
                fwrite(&num_of_coeff_per_plane, 2, 1, fh);
                fwrite(coeff_rle, 4, num_of_coeff_per_plane, fh);
            }

            binary_check_and_write_end_element("Plane", 0x05);
        }
    }
    //binary_write_end_element();

    return;


}


void addCoeffsToTraceAndGenHistogram(Macroblock* currMB, Slice* currSlice, int isIntraFrame)
{
    if(isIntraFrame==0){
        return;
    }
    //initArray(&histogramArray,100);
    //initMap(&dct_coeff_to_index,100);

    if(previous_frame_id!=iCurr_frame_id){
        /*I'm writing a new frame*/
        freeArray(&histogramArray);
        initArray(&histogramArray,100);

        freeMap(&dct_coeff_to_index);
        initMap(&dct_coeff_to_index,100);

        freeArray(&histogramCbArray);
        initArray(&histogramCbArray,100);

        freeMap(&dct_coeff_to_index_cb);
        initMap(&dct_coeff_to_index_cb,100);

        freeArray(&histogramCrArray);
        initArray(&histogramCrArray,100);

        freeMap(&dct_coeff_to_index_cr);
        initMap(&dct_coeff_to_index_cr,100);


        previous_frame_id = iCurr_frame_id;
        current_frame_name = toArray(iCurr_frame_id);
    }
    int pl, i, j;
    xml_write_start_element("Coeffs");

    const char* name = "dct_histogram_luma_frame_";
    const char* cr_name = "dct_histogram_cb_frame_";
    const char* cb_name = "dct_histogram_cr_frame_";

    const char* ext = ".json";


    char* name_with_extension;
    name_with_extension = malloc(strlen(name)+strlen(current_frame_name)+strlen(ext));
    strcpy(name_with_extension, name);
    strcat(name_with_extension, current_frame_name);
    strcat(name_with_extension, ext);

    char* name_with_extension_cb;
    name_with_extension_cb = malloc(strlen(cb_name)+strlen(current_frame_name)+strlen(ext));
    strcpy(name_with_extension_cb, cb_name);
    strcat(name_with_extension_cb, current_frame_name);
    strcat(name_with_extension_cb, ext);

    char* name_with_extension_cr;
    name_with_extension_cr = malloc(strlen(cr_name)+strlen(current_frame_name)+strlen(ext));
    strcpy(name_with_extension_cr, cr_name);
    strcat(name_with_extension_cr, current_frame_name);
    strcat(name_with_extension_cr, ext);

    if(currMB->luma_transform_size_8x8_flag)
    {
        //Luma
        xml_write_start_element("Plane");
        xml_write_int_attribute("type", 0);


        if (!fptr) perror("fopen");
        for(i = 0; i < 16; i++)
        {
            //printf("%c",'H');
            xml_write_start_element("Row");
            for(j = 0; j < 16; j++)
            {
                if(j > 0) {
                    xml_write_text(",");
                }
                xml_write_int((&currSlice->mb_rres[0][0])[i][j]);

                int found = 0;

                for(int idx_M = 0; idx_M<dct_coeff_to_index.used; idx_M++){
                    if(dct_coeff_to_index.array[idx_M].dct_coeff==(&currSlice->mb_rres[0][0])[i][j]){
                        histogramArray.array[dct_coeff_to_index.array[idx_M].histogram_array_index]++;
                        found = 1;
                    }
                }
                if(found==0){
                    insertArray(&histogramArray,1);
                    pair_int_int* p = malloc(sizeof(pair_int_int));
                    p->histogram_array_index = (int)histogramArray.used-1;
                    p->dct_coeff = (&currSlice->mb_rres[0][0])[i][j];
                    insertMap(&dct_coeff_to_index,*p);
                }
            }
            xml_write_end_element();
        }


       /* fptr = fopen(name_with_extension,"w");
        saveHistogram(fptr, &histogramArray, &dct_coeff_to_index);
        fclose(fptr);*/

        xml_write_end_element();
        //Chroma

        for(pl = 1; pl <= 2; pl++)
        {

            xml_write_start_element("Plane");
            xml_write_int_attribute("type", pl);
            for( i = 0; i < 8; i++)
            {
                xml_write_start_element("Row");
                for(j = 0; j < 8; j++)
                {
                    if(j > 0) xml_write_text(",");
                    xml_write_int(currSlice->cof[pl][i][j]);

                    if(pl==1){

                        //CB histogram
                        int found = 0;

                        for(int idx_M = 0; idx_M<dct_coeff_to_index_cb.used; idx_M++){
                            if(dct_coeff_to_index_cb.array[idx_M].dct_coeff==currSlice->cof[pl][i][j]){
                                histogramCbArray.array[dct_coeff_to_index_cb.array[idx_M].histogram_array_index]++;
                                found = 1;
                            }
                        }
                        if(found==0){
                            insertArray(&histogramCbArray,1);
                            pair_int_int* p = malloc(sizeof(pair_int_int));
                            p->histogram_array_index = (int)histogramCbArray.used-1;
                            p->dct_coeff = currSlice->cof[pl][i][j];
                            insertMap(&dct_coeff_to_index_cb,*p);
                        }
                    }else{

                        //CR histogram
                        int found = 0;

                        for(int idx_M = 0; idx_M<dct_coeff_to_index_cr.used; idx_M++){
                            if(dct_coeff_to_index_cr.array[idx_M].dct_coeff==currSlice->cof[pl][i][j]){
                                histogramCrArray.array[dct_coeff_to_index_cr.array[idx_M].histogram_array_index]++;
                                found = 1;
                            }
                        }
                        if(found==0){
                            insertArray(&histogramCrArray,1);
                            pair_int_int* p = malloc(sizeof(pair_int_int));
                            p->histogram_array_index = (int)histogramCrArray.used-1;
                            p->dct_coeff = currSlice->cof[pl][i][j];
                            insertMap(&dct_coeff_to_index_cr,*p);
                        }
                    }

                }
                xml_write_end_element();
            }
            xml_write_end_element();
        }

       /* fptr = fopen(name_with_extension_cb,"w");
        saveHistogram(fptr, &histogramCbArray, &dct_coeff_to_index_cb);
        fclose(fptr);

        fptr = fopen(name_with_extension_cr,"w");
        saveHistogram(fptr, &histogramCrArray, &dct_coeff_to_index_cr);
        fclose(fptr);*/

    }
    else
    {
        //Luma
        xml_write_start_element("Plane");
        xml_write_int_attribute("type", 0);
        for(i = 0; i < 16; i++)
        {
            xml_write_start_element("Row");
            for(j = 0; j < 16; j++)
            {
                if(j > 0) xml_write_text(",");
                xml_write_int(currSlice->cof[0][i][j]);
                int found = 0;

                for(int idx_M = 0; idx_M<dct_coeff_to_index.used; idx_M++){
                    if(dct_coeff_to_index.array[idx_M].dct_coeff==currSlice->cof[0][i][j]){
                        histogramArray.array[dct_coeff_to_index.array[idx_M].histogram_array_index]++;
                        found = 1;
                    }
                }
                if(found==0){
                    insertArray(&histogramArray,1);
                    pair_int_int* p = malloc(sizeof(pair_int_int));
                    p->histogram_array_index = (int)histogramArray.used-1;
                    p->dct_coeff = currSlice->cof[0][i][j];
                    insertMap(&dct_coeff_to_index,*p);
                }

            }
            xml_write_end_element();
        }
        xml_write_end_element();

      /*  fptr = fopen(name_with_extension,"w");
        saveHistogram(fptr, &histogramArray, &dct_coeff_to_index);
        fclose(fptr);*/

        //Chroma
        for(pl = 1; pl <= 2; pl++)
        {
            xml_write_start_element("Plane");
            xml_write_int_attribute("type", pl);
            for( i = 0; i < 8; i++)
            {
                xml_write_start_element("Row");
                for(j = 0; j < 8; j++)
                {
                    if(j > 0) xml_write_text(",");
                    xml_write_int(currSlice->cof[pl][i][j]);

                    if(pl==1){

                        //CB histogram
                        int found = 0;

                        for(int idx_M = 0; idx_M<dct_coeff_to_index_cb.used; idx_M++){
                            if(dct_coeff_to_index_cb.array[idx_M].dct_coeff==currSlice->cof[pl][i][j]){
                                histogramCbArray.array[dct_coeff_to_index_cb.array[idx_M].histogram_array_index]++;
                                found = 1;
                            }
                        }
                        if(found==0){
                            insertArray(&histogramCbArray,1);
                            pair_int_int* p = malloc(sizeof(pair_int_int));
                            p->histogram_array_index = (int)histogramCbArray.used-1;
                            p->dct_coeff = currSlice->cof[pl][i][j];
                            insertMap(&dct_coeff_to_index_cb,*p);
                        }
                    }else{

                        //CR histogram
                        int found = 0;

                        for(int idx_M = 0; idx_M<dct_coeff_to_index_cr.used; idx_M++){
                            if(dct_coeff_to_index_cr.array[idx_M].dct_coeff==currSlice->cof[pl][i][j]){
                                histogramCrArray.array[dct_coeff_to_index_cr.array[idx_M].histogram_array_index]++;
                                found = 1;
                            }
                        }
                        if(found==0){
                            insertArray(&histogramCrArray,1);
                            pair_int_int* p = malloc(sizeof(pair_int_int));
                            p->histogram_array_index = (int)histogramCrArray.used-1;
                            p->dct_coeff = currSlice->cof[pl][i][j];
                            insertMap(&dct_coeff_to_index_cr,*p);
                        }
                    }

                }
                xml_write_end_element();
            }
            xml_write_end_element();
        }
       /* fptr = fopen(name_with_extension_cb,"w");
        saveHistogram(fptr, &histogramCbArray, &dct_coeff_to_index_cb);
        fclose(fptr);

        fptr = fopen(name_with_extension_cr,"w");
        saveHistogram(fptr, &histogramCrArray, &dct_coeff_to_index_cr);
        fclose(fptr);*/

    }
    xml_write_end_element();
}

void writeMBInfo(Macroblock* currMB, Slice* currSlice)
{
	char typestring[255];
	char predmodstring[255];
	switch (currSlice->slice_type)
	{
		case P_SLICE:
			getMbTypeName_P_SP_Slice(iCurr_mb_type, currMB, typestring, predmodstring, 0);
			break;
		case SP_SLICE:
			getMbTypeName_P_SP_Slice(iCurr_mb_type, currMB, typestring, predmodstring, 0);
			break;
		case B_SLICE:
			getMbTypeName_B_Slice(iCurr_mb_type, currMB, typestring, predmodstring, 0);
			break;
		case I_SLICE:
			getMbTypeName_I_Slice(iCurr_mb_type, currMB, typestring, predmodstring, 0);
			break;
		case SI_SLICE:
			getMbTypeName_SI_Slice(iCurr_mb_type, currMB, typestring, predmodstring, 0);
			break;
	}
	//xml_write_start_element("Type");
    
    unsigned char macroblock_type;

		switch (currSlice->slice_type)
		{
			case P_SLICE:
			case SP_SLICE:
                if (currMB->skip_flag == 1)
                    macroblock_type = 50;
					//xml_write_int(-1);
				else
				{
                    if (iCurr_mb_type != 0)
                        macroblock_type = iCurr_mb_type - 1;
						//xml_write_int(iCurr_mb_type-1);
					else
                        if (iCurr_mb_type == 5) {
                            if (currMB->luma_transform_size_8x8_flag == 0)
                                macroblock_type = 48; //I_4x4
                            else
                                macroblock_type = 49; //I_8x8
                        }
                        else
                            macroblock_type = iCurr_mb_type;
						//xml_write_int(iCurr_mb_type);
				}
				break;
			case B_SLICE:
                if (currMB->skip_flag == 1)
                    //xml_write_int(-1);
                    macroblock_type = 53;
				else
                    if (iCurr_mb_type == 23) {
                        if (currMB->luma_transform_size_8x8_flag == 0)
                            macroblock_type = 51; //I_4x4
                        else
                            macroblock_type = 52; //I_8x8
                    }
                    else
                        macroblock_type = iCurr_mb_type;
					//xml_write_int(iCurr_mb_type);
				break;
			case I_SLICE:
			case SI_SLICE:
                macroblock_type = iCurr_mb_type;
                if (iCurr_mb_type == 0) {
                    if (currMB->luma_transform_size_8x8_flag == 0)
                        macroblock_type = 48; //I_4x4
                    else
                        macroblock_type = 49; //I_8x8
                }
				//xml_write_int(iCurr_mb_type);
				break;
		}

        fwrite(&macroblock_type, 1, 1, fh);

	/*xml_write_end_element();
	xml_write_start_element("TypeString");
		xml_write_text(typestring);
	xml_write_end_element();*/
    

	//xml_write_start_element("PredModeString");
	//	xml_write_text(predmodstring);
	//xml_write_end_element();

        /*switch (predmodstring) {
        case "BLOCK_TYPE_I":
            break;
        case 
    }*/

	/*xml_write_start_element("SkipFlag");
		if(typestring[0] == 'I')
			xml_write_int(0);
		else
			xml_write_int(currMB->skip_flag);
	xml_write_end_element();*/
}

void writeNALInfo(Slice* currSlice)
{
	char typestring[256];

	//What about slice type 20?
	//Write NonPicture element when needed
	//if(curr_NAL->nal_unit_type > 5)
	//{
		//binary_check_and_write_end_element(0x01); //End the picture tag when needed
		//xml_write_start_element("NonPicture");
	//}

	/*xml_write_start_element("NAL");
		xml_write_start_element("Num");
			if(dpc_NAL != NULL && dpb_NAL != NULL)
			{
				xml_write_int(iCurr_NAL_number - 2);
			}
			else
			{
				if(dpc_NAL != NULL || dpb_NAL != NULL)
					xml_write_int(iCurr_NAL_number - 1);
				else
					xml_write_int(iCurr_NAL_number);
			}
		xml_write_end_element();
		xml_write_start_element("Type");
			xml_write_int(curr_NAL->nal_unit_type);
		xml_write_end_element();
		xml_write_start_element("TypeString");
			getNALTypeName(curr_NAL->nal_unit_type, typestring);
			xml_write_text(typestring);
		xml_write_end_element();
		xml_write_start_element("Length");
			xml_write_int(curr_NAL->len);
		xml_write_end_element();
	xml_write_end_element();
*/

	////Write DPB NAL info
	//if(currSlice->dpB_NotPresent == 0 && dpb_NAL != NULL)
	//{
	//	xml_write_start_element("NAL");
	//		xml_write_start_element("Num");
	//			if(dpc_NAL != NULL)
	//				xml_write_int(iCurr_NAL_number - 1);
	//			else
	//				xml_write_int(iCurr_NAL_number);
	//		xml_write_end_element();
	//		xml_write_start_element("Type");
	//			xml_write_int(dpb_NAL->nal_unit_type);
	//		xml_write_end_element();
	//		xml_write_start_element("TypeString");
	//			getNALTypeName(dpb_NAL->nal_unit_type, typestring);
	//			xml_write_text(typestring);
	//		xml_write_end_element();
	//		xml_write_start_element("Length");
	//			xml_write_int(dpb_NAL->len);
	//		xml_write_end_element();
	//	xml_write_end_element();
	//}

	////Write DPC NAL info
	//if(currSlice->dpC_NotPresent == 0 && dpc_NAL != NULL)
	//{
	//	xml_write_start_element("NAL");
	//		xml_write_start_element("Num");
	//			xml_write_int(iCurr_NAL_number);
	//		xml_write_end_element();
	//		xml_write_start_element("Type");
	//			xml_write_int(dpc_NAL->nal_unit_type);
	//		xml_write_end_element();
	//		xml_write_start_element("TypeString");
	//			getNALTypeName(dpc_NAL->nal_unit_type, typestring);
	//			xml_write_text(typestring);
	//		xml_write_end_element();
	//		xml_write_start_element("Length");
	//			xml_write_int(dpc_NAL->len);
	//		xml_write_end_element();
	//	xml_write_end_element();
	//}

	//if(curr_NAL->nal_unit_type > 5) xml_write_end_element();
}

void getNALTypeName(int nal_type, char* typestring)
{
	switch(nal_type)
	{
		case 0: strcpy(typestring, "Unspecified"); break;
		case 1: strcpy(typestring, "NALU_TYPE_SLICE"); break;
		case 2: strcpy(typestring, "NALU_TYPE_DPA"); break;
		case 3: strcpy(typestring, "NALU_TYPE_DPB"); break;
		case 4: strcpy(typestring, "NALU_TYPE_DPC"); break;
		case 5: strcpy(typestring, "NALU_TYPE_IDR"); break;
		case 6: strcpy(typestring, "NALU_TYPE_SEI"); break;
		case 7: strcpy(typestring, "NALU_TYPE_SPS"); break;
		case 8: strcpy(typestring, "NALU_TYPE_PPS"); break;
		case 9: strcpy(typestring, "NALU_TYPE_AUD"); break;
		case 10: strcpy(typestring, "NALU_TYPE_EOSEQ"); break;
		case 11: strcpy(typestring, "NALU_TYPE_EOSTREAM"); break;
		case 12: strcpy(typestring, "NALU_TYPE_FILL"); break;
		case 13: strcpy(typestring, "NALU_TYPE_SPSEXT"); break;
		case 14: strcpy(typestring, "NALU_TYPE_PREFIX"); break;
		case 15: strcpy(typestring, "NALU_TYPE_SPSSUBSET"); break;
		case 16:
		case 17:
		case 18: strcpy(typestring, "Reserved"); break;
		case 19: strcpy(typestring, "NALU_TYPE_AUX"); break;
		case 20: strcpy(typestring, "NALU_TYPE_SLICEEXT"); break;
		case 21:
		case 22:
		case 23: strcpy(typestring, "Reserved"); break;
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 31: strcpy(typestring, "Unspecified"); break;
		default: strcpy(typestring, "UNKNOWN"); break;
	}
}

void setCurrentMBType(int value)
{
	iCurr_mb_type = value;
}

