#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <jpeglib.h>
#include "jpegUtils.h"


struct jpeg_compress_struct cinfo;
struct jpeg_decompress_struct dinfo;

struct jpeg_error_mgr       jcerr,jderr;
 

jvirt_barray_ptr * data = NULL;


void finishDecoding(void){

	jpeg_destroy_decompress(&dinfo);

}
void finishEncoding(void){

	jpeg_destroy_compress(&cinfo);
}



void ju_prepare()
{


    dinfo.err = jpeg_std_error(&jderr);
    jpeg_create_decompress(&dinfo);
    cinfo.err = jpeg_std_error(&jcerr);
    jpeg_create_compress(&cinfo);

    //2 MB to both
    cinfo.mem->max_memory_to_use = 2*1024*1024;
    dinfo.mem->max_memory_to_use = 2*1024*1024;
}



void prepareDecoding(unsigned char* inJpg, unsigned long inJpgSize)
{

	//dinfo.raw_data_out=TRUE;
	//printf ("here1");
	jpeg_mem_src(&dinfo,inJpg,inJpgSize);
	//jpeg_start_decompress(&dinfo);
	//printf ("here");
	jpeg_read_header(&dinfo,TRUE);
 	data = jpeg_read_coefficients(&dinfo);
	//printf ("pass");





}

void prepareEncoding(unsigned char** outJpg, unsigned long* outJpgSize)
{




	
	//jpeg_set_quality(&cinfo, 80, TRUE); //todo upgrade

	jpeg_copy_critical_parameters(&dinfo, &cinfo);
	//jpeg_set_defaults(&cinfo);
	jpeg_simple_progression(&cinfo);
  	//cinfo.image_width = dinfo.image_width; 	/* image width and height, in pixels */
  	//cinfo.image_height = dinfo.image_height;
  	//cinfo.input_components = dinfo.input_components;/* # of color components per pixel */
  	//cinfo.in_color_space = dinfo.in_color_space; 	/* colorspace of input image */

	//cinfo.dct_method = JDCT_FASTEST; 
	
	//cinfo.raw_data_in = TRUE; // Supply downsampled data 
	jpeg_mem_dest(&cinfo,outJpg,outJpgSize);
	//jpeg_start_compress(&cinfo, TRUE);
	//jpeg_write_header(&cinfo);
	//printf ("pass2");
 	
}

void copyData()
{
	jpeg_write_coefficients(&cinfo,data);
}



int ju_processFrame(unsigned char* inJpg, unsigned long inJpgSize, unsigned char** outJpg, unsigned long* outJpgSize)
{
	prepareDecoding(inJpg,inJpgSize);
	prepareEncoding(outJpg,outJpgSize);
	
	copyData();


    // should be first to copy coefficients data
    jpeg_finish_compress(&cinfo);
    jpeg_finish_decompress(&dinfo);

	return 0;//TODO error handling!!!
}

#define MARKER_START ((unsigned char)0xFF)
#define MARKER_EOI ((unsigned char)0xD9)
#define MARKER_SOS ((unsigned char)0xDA)
#define MARKER_REGULAR ((unsigned char)0xC0)
#define MARKER_REGULAR_EXT ((unsigned char)0xC1)
#define MARKER_PROGRESSIVE ((unsigned char)0xC2)


unsigned long ju_cropJpeg (unsigned char* jpg, unsigned long jpgSize, unsigned long maxCropSize)
{
	if(maxCropSize>=jpgSize)
	{
		return jpgSize;
	}
	
	int sectionStart=0;
	int prevSectionStart=0;
	


	for(int i=0;i<maxCropSize-1;i++)
	{	
		if(jpg[i]==MARKER_START)
            	{ 
			switch(jpg[i+1]){
			//what if...
			case MARKER_EOI:
				// end of JPEG found!
		  		return i+1;
			case MARKER_SOS:
				prevSectionStart=sectionStart;
				sectionStart=i;
			}

            	}
	}
	if(sectionStart==0)
	{
		// not a progressive jpeg, unable to crop
		return jpgSize;
	}

	// if we are here, jpeg is bigger than needed
	if((prevSectionStart==0) && (sectionStart!=0) )// we are still in first section
	{
		// nothing to crop, scanning to next section
		bool sectionFound = FALSE;				
		for(int i=maxCropSize-1;i<jpgSize-1;i++)
		{	
			
			if(jpg[i]==MARKER_START)
            		{ 
				switch(jpg[i+1]){
				//what if...
				case MARKER_EOI:
					// end of JPEG found!
		  			return i+1;
				case MARKER_SOS:
					prevSectionStart=sectionStart;
					sectionStart=i;
					sectionFound = TRUE;
				}

            		}
			if(sectionFound) break;
		}
		
		
	}

	// now we have start and end of last good section
	// replacing next SOS marker with EOI marker
	jpg[sectionStart+1]=MARKER_EOI;
	return sectionStart+1;
	
}



//#define MARKER_REGULAR ((unsigned char)0xC0)
//#define MARKER_REGULAR_EXT ((unsigned char)0xC1)
//#define MARKER_PROGRESSIVE ((unsigned char)0xC2)


int ju_checkIfJpeg(unsigned char* jpg, unsigned long jpgSize)
{
    for(int i=0;i<jpgSize-1;i++)
    {
        if(jpg[i]==MARKER_START)
        {
            switch(jpg[i+1]){
            //what if...
            case MARKER_REGULAR:
            case MARKER_REGULAR_EXT:
                // regular
                return JPEG_GOOD_REGULAR;
            case MARKER_PROGRESSIVE:
                return JPEG_GOOD_PROGRESSIVE;
            }

        }
    }
    return JPEG_BAD;
}





void ju_cleanup()
{
	jpeg_destroy_decompress(&dinfo);
	jpeg_destroy_compress(&cinfo);
}
