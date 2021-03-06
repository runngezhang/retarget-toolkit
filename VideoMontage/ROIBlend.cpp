#include "stdafx.h"
#include "ROIBlend.h"

void ROIBlend::cvShowManyImages(char* title, int nArgs, ...) {

    // img - Used for getting the arguments 
    IplImage *img;
	CvPoint p1, p2, p3;

    // DispImage - the image in which input images are to be copied
    IplImage *DispImage;

    int size;
    int i;
    int m, n;
    int x, y;

    // w - Maximum number of images in a row 
    // h - Maximum number of images in a column 
    int w, h;

    // scale - How much we have to resize the image
    float scale;
    int max;

    // If the number of arguments is lesser than 0 or greater than 12
    // return without displaying 
    if(nArgs <= 0) {
        printf("Number of arguments too small....\n");
        return;
    }
    else if(nArgs > 12) {
        printf("Number of arguments too large....\n");
        return;
    }
    // Determine the size of the image, 
    // and the number of rows/cols 
    // from number of arguments 
    else if (nArgs == 1) {
        w = h = 1;
        size = 300;
    }
	
    // Create a new 3 channel image
    DispImage = cvCreateImage( cvSize(size*w, size*h), 8, 3 );

    // Used to get the arguments passed
    va_list args;
    va_start(args, nArgs);

    // Loop for nArgs number of arguments
    for (i = 0, m = 20, n = 20; i < nArgs; i++, m += (size)) {

        // Get the Pointer to the IplImage
        img = va_arg(args, IplImage*);

        // Check whether it is NULL or not
        // If it is NULL, release the image, and return
        if(img == 0) {
            printf("Invalid arguments");
            cvReleaseImage(&DispImage);
            return;
        }

        // Find the width and height of the image
        x = img->width;
        y = img->height;

        // Find whether height or width is greater in order to resize the image
        max = (x > y)? x: y;

        // Find the scaling factor to resize the image
        scale = (float) ( (float) max / size );

        // Used to Align the images
        if( i % w == 0 && m!= 20) {
            //m = 20;
            //n+= 20 + size;
			m = 20;
			n += y;
        }		
		
        // Set the image ROI to display the current image
        cvSetImageROI(DispImage, cvRect(m, n, (int)( x/scale ), (int)( y/scale )));
		
        // Resize the input image and copy it to the Single Big Image
        cvResize(img, DispImage);

        // Reset the ROI in order to display the next image
        cvResetImageROI(DispImage);						

		/*p1.x = m;
		p1.y = n;
		p2.x = m;
		p2.y = (int)( y/scale )+20;
		

		cvLine(DispImage, p1, p2, CV_RGB(0,0,255), 2, 8, 0);*/

		p1.x = m-20;
		p1.y = n;
		p2.x = m;
		p2.y = (int)( y/scale )+20;
		p3.x = m+20;
		p3.y = n;
		

		cvRectangle(DispImage, p1, p2, CV_RGB(0,0,255), 2, 8, 0);
		cvRectangle(DispImage, p3, p2, CV_RGB(255,0,0), 2, 8, 0);

		/*printf("p1x=%d\np1y=%d\np2x=%d\np2y=%d\n", p1.x, p1.y, p2.x, p2.y);*/	
    }		 		

    // Create a new window, and show the Single Big Image
    cvNamedWindow( title, 1 );
    cvShowImage( title, DispImage);

    cvWaitKey();
    cvDestroyWindow(title);

    // End the number of arguments
    va_end(args);

    // Release the Image Memory
    cvReleaseImage(&DispImage);	
}
double ROIBlend::calcFsofP(int p, int mean)
{
	double pi = 3.1415926535;
	return Gaussian(mean, _variance, p) / ( sqrt(2 * pi) * _variance);
}

double ROIBlend::calcMean(int p, int a, int b, double scale)
{
	return scale * (a + pow((double)p - a, 2.0) / pow((double)b - p, 2) * p);	
}

double ROIBlend::calcDefiniteIntergral(int p, int start, int end)
{
	double result = 0;
	// calculate mean depends on pixel position
	double mean = calcMean(p, _a, _b, _scale);
	
	// convert start, end to (0, 1) distribution based on mean and variance
	start = (310 - mean) / 20;
	end = (end - mean) / _variance;

	double start_cummulative = NormalDistribution(start);
	double end_cummulative = NormalDistribution(end); 
	return end_cummulative - start_cummulative;
}

CvScalar ROIBlend::GetBlendScalar(CvScalar value, CvScalar seam, int p, bool is_S)
{
	CvScalar blendValue;					// result
	for(int i = 0; i < 4; i++)
	{
		double Is_p = value.val[i];			// value of Is(p)
		double It_p = seam.val[i];			// value of It(p) which is actually It(seam)
		
		double Ps_p;
		if(is_S)
			Ps_p = calcDefiniteIntergral(p, _a, p); // value of Ps(p)
		else
			Ps_p = calcDefiniteIntergral(p, p, _b); // value of Ps(p)

		double Pt_p = 1 - Ps_p;				// value of Pt(p)
		double Is_pb = Is_p * Ps_p + It_p * Pt_p;
		blendValue.val[i] = Is_pb;
	}		
	return blendValue;
}

CvScalar ROIBlend::GetBlendScalar2(CvScalar value, CvScalar seam, int p, int deviation)
{
	CvScalar blendValue;					// result
	for(int i = 0; i < 4; i++)
	{
		double Is_p = value.val[i];			// value of Is(p)
		double It_p = seam.val[i];			// value of It(p) which is actually It(seam)
		
		double Ps_p;
		// integrate from -inf to end
		double end =  (double)p / deviation;
		Ps_p = NormalDistribution(end);
		double Pt_p = 1 - Ps_p;				// value of Pt(p)
		double Is_pb = Is_p * Ps_p + It_p * Pt_p;
		blendValue.val[i] = Is_pb;
	}		
	return blendValue;
}

IplImage* ROIBlend::CombineImages(IplImage* image1, IplImage* image2)
{
	IplImage* image = cvCreateImage(
		cvSize(image1->width + image2->width, image1->height), 
		image1->depth, image1->nChannels);
	
	for(int x = 0; x < image1->width; x++)
		for(int y = 0; y < image1->height; y++)
		{
			CvScalar value = cvGet2D(image1, y, x);
			cvSet2D(image, y, x, value);
		}
	for(int x = 0; x < image2->width; x++)
		for(int y = 0; y < image2->height; y++)
		{
			CvScalar value = cvGet2D(image2, y, x);
			cvSet2D(image, y, x + image1->width, value);
		}
	return image;
}


IplImage* ROIBlend::BlendImages1(IplImage* image1, IplImage* image2, int a, int b)
{	 
	int width1 = image1->width;
	int height1 = image1->height;
	int width2 = image2->width;
	int height2 = image2->height;

	// update _a and _b to the coordinate after blended
	_a = width1 - a;
	_b = width1 + b;

	IplImage* image1_clone = cvCloneImage(image1);
	IplImage* image2_clone = cvCloneImage(image2);

	IplImage* image = cvCreateImage(cvSize(width1 + width2, height1), image1->depth, image1->nChannels);

	// for S part
	for(int x = _a; x < width1; x++)
	{
		for(int y = 0; y < height1; y++)
		{
			CvScalar value = cvGet2D(image1, y, x);
			CvScalar seamValue = cvGet2D(image2, y, 0);
			CvScalar blendValue = GetBlendScalar(value, seamValue, x, true);
			cvSet2D(image1_clone, y, x, blendValue);
		}
	}
	
	// for T part
	for(int x = 0; x < _b - width1; x++)
	{
		for(int y = 0; y < height2; y++)
		{
			CvScalar value = cvGet2D(image2, y, x);
			CvScalar seamValue = cvGet2D(image1, y, width1 - 1);
			CvScalar blendValue = GetBlendScalar(value, seamValue, x + width1, false);
			cvSet2D(image2_clone, y, x, blendValue);
		}
	} 
	return CombineImages(image1_clone, image2_clone);	 
}

IplImage* ROIBlend::BlendImages2(IplImage* image1, IplImage* image2, int a, int b)
{	 
	int width1 = image1->width;
	int height1 = image1->height;
	int width2 = image2->width;
	int height2 = image2->height;

	_a = a;
	_b = b;

	IplImage* image1_clone = cvCloneImage(image1);
	IplImage* image2_clone = cvCloneImage(image2);

	IplImage* image = cvCreateImage(cvSize(width1 + width2, height1), image1->depth, image1->nChannels);

	// for S part
	for(int x = width1 - _a - 30; x < width1; x++)
	{
		for(int y = 0; y < height1; y++)
		{
			CvScalar value = cvGet2D(image1, y, x);
			CvScalar seamValue = cvGet2D(image2, y, 0);
			CvScalar blendValue = GetBlendScalar2(value, seamValue, width1 - x, _a);
			cvSet2D(image1_clone, y, x, blendValue);
		}
	}
	
	// for T part
	for(int x = 0; x < _b + 30; x++)
	{
		for(int y = 0; y < height2; y++)
		{
			CvScalar value = cvGet2D(image2, y, x);
			CvScalar seamValue = cvGet2D(image1, y, width1 - 1);
			CvScalar blendValue = GetBlendScalar2(value, seamValue, x, _b);
			cvSet2D(image2_clone, y, x, blendValue);
		}
	}
	return CombineImages(image1_clone, image2_clone);	 
}

IplImage* ROIBlend::BlendImages3(IplImage* image1, IplImage* image2, int size, int std)
{	
	int width1 = image1->width;
	int height1 = image1->height;
	int width2 = image2->width;
	int height2 = image2->height;

	// get non-overlap part in image1	
	IplImage* nonoverlap1 = cvCreateImage(cvSize(width1 - size, height1), image1->depth, image1->nChannels);	 
	cvSetImageROI(image1, cvRect(0, 0, width1 - size, height1));
	cvCopyImage(image1, nonoverlap1);	
	
	// get overlap area in image1
	cvSetImageROI(image1, cvRect(width1 - size, 0, size, height1));
	IplImage* overlap1 = cvCreateImage(cvSize(size, height1), image1->depth, image1->nChannels);
	cvCopyImage(image1, overlap1);

	// get the first part of 2nd image
	cvSetImageROI(image2, cvRect(0,0, size, height2));
	IplImage* overlap2 = cvCreateImage(cvSize(size, height2), image2->depth, image2->nChannels);
	cvCopyImage(image2, overlap2);

	// get non-overlap area in image2
	IplImage* nonoverlap2 = cvCreateImage(cvSize(width2 - size, height1), image2->depth, image2->nChannels);	 
	cvSetImageROI(image2, cvRect(size, 0, width2 - size, height2));
	cvCopyImage(image2, nonoverlap2);	

	// blend 2 overlap area using Gaussian
	IplImage* blendArea = cvCreateImage(cvSize(size, height2), image2->depth, image2->nChannels);
	int mean = size / 2;
	for(int x = 0; x < size; x++)
	{
		for(int y = 0; y < height1; y++)
		{
			CvScalar value1 = cvGet2D(overlap1, y, x);
			CvScalar value2 = cvGet2D(overlap2, y, x);
			CvScalar value = GetBlendScalar2(value1, value2, mean - x, std);
			cvSet2D(blendArea, y, x, value);
		}
	}

	// combine 2 images
	IplImage* firstPart = CombineImages(nonoverlap1, blendArea);
	IplImage* result = CombineImages(firstPart, nonoverlap2);
	
	// release if using OpenCV 1.0
	// here we use OpenCV 2.0 so no release is needed.
	
	cvReleaseImage(&overlap1);
	cvReleaseImage(&overlap2);
	cvReleaseImage(&nonoverlap1);
	cvReleaseImage(&nonoverlap2);
	cvReleaseImage(&firstPart);
	return result;
}

void ROIBlend::TestBlendMultiple(char **fileList, int count, int para1, int para2, int blendType)
{	
	if(count > 1)
	{	
		IplImage* result;
		int minHeight = int::MaxValue;
		IplImage** imageList = (IplImage**)malloc(count * sizeof(long));;
		for(int i = 0; i < count; i++)
		{
			IplImage* image = cvLoadImage(fileList[i]);
			if(image == 0)
				printf("error");
				
			imageList[i] = image;
			int height = image->height;
			if(height < minHeight) minHeight = height;		
		}		
		
		// ********resize then blend
		// first image
		double ratio = (double)minHeight/ (double)imageList[0]->height;
		result = cvCreateImage(cvSize(imageList[0]->width * ratio, minHeight), 
			imageList[0]->depth, imageList[0]->nChannels);
		cvResize(imageList[0], result);
		// from second image
		for(int i = 1; i < count; i++)
		{
			IplImage* temp = cvCreateImage(cvSize(imageList[i]->width * ratio, minHeight), 
			imageList[i]->depth, imageList[i]->nChannels);
			cvResize(imageList[i], temp);

			switch(blendType)
			{
			case 1: result = BlendImages1(result, temp, para1, para2);
				break;
			case 2: result = BlendImages2(result, temp, para1, para2);
				break;
			case 3: result = BlendImages3(result, temp, para1, para2);
				break;
			default:
				result = BlendImages3(result, temp, para1, para2);
			}
		}

		cvSaveImage("blend.jpg", result);
		cvNamedWindow("Blended");
		while(1)
		{
			cvShowImage("Blended", result);
			int key = cvWaitKey(100);
			if(key == 27) break;
		}
	}	
}

// old version
void ROIBlend::cvBlendImages(IplImage *blend1)
{
	int width, height, rmean, gmean, bmean, rt=0, bt=0, gt=0, clw=0, rvar=0, gvar=0, bvar=0;
	long rvartot=0, gvartot=0, bvartot=0;
	width = blend1->width;
	height = blend1->height;	

	// Reads the pixel values of the image within the given width of 20 pixels from the edge of the image
	// Calculate the mean of the 20 pixel width
	CvScalar s;
	for(int i=0;i<height;i++){
		for(int j=width-20;j<width;j++){
			s = cvGet2D(blend1, i, j);
			
			//Sum of each channel for use in calculating the mean
			rt += s.val[2];
			gt += s.val[1];
			bt += s.val[0];
			clw++;
			//printf("B=%f, G=%f, R=%f\n",s.val[0],s.val[1],s.val[2]);			
		}		
	}
	
	//Calculate the mean of each channel
	rmean = rt/clw;
	bmean = bt/clw;
	gmean = gt/clw;

	//printf("R mean=%d\nG mean=%d\nB mean=%d\n",rmean, gmean, bmean);

	//Calculate the variance of the same area
	for(int i=0;i<height;i++)
	{
		for(int j=width-20; j<width;j++)
		{
			s = cvGet2D(blend1, i, j);
			rvartot += pow((s.val[2]-rmean),2);				
			gvartot += pow((s.val[1]-rmean),2);			
			bvartot += pow((s.val[0]-rmean),2);			
		}
	}
	
	//Variance mean of the same area
	rvar = rvartot/clw;
	gvar = gvartot/clw;
	bvar = bvartot/clw;

	//printf("R var=%d\nG var=%d\nB var=%d\nTotal=%d\n",rvartot, gvartot, bvartot, clw);

	float r, g, b, rval, gval, bval;
	rval=gval=bval=0;
	for(int i=0;i<height;i++)
	{
		for(int j=width-20; j<width;j++)
		{
			s = cvGet2D(blend1, i, j);
			r = s.val[2];
			g = s.val[1];
			b = s.val[0];
						
			rval = pow((double)(r - rmean),2)/(2*rvar);			
			gval = pow((double)(g - gmean),2)/(2*gvar);
			bval = pow((double)(b - bmean),2)/(2*bvar);

			// Function call to calculate the value of the pixels through a Gaussian distribution
			//calcFsofP(rval, rvar, 'R');
			//calcFsofP(gval, gvar, 'G');
			//calcFsofP(bval, bvar, 'B');
		}
	}
}





void TestCombineImages()
{
	IplImage* image1 = cvLoadImage("test1.jpg");
	IplImage* image2 = cvLoadImage("test2.jpg");
	ROIBlend* roiBlend = new ROIBlend(2.0, 2.0); // dummy parameters
	IplImage* image = roiBlend->CombineImages(image1, image2);

	cvNamedWindow("Test");

	while(1)
	{
		cvShowImage("Test", image);
		cvWaitKey(100);
	}
}


void TestBlendImages()
{
	IplImage* image1 = cvLoadImage("test1.jpg");
	IplImage* image2 = cvLoadImage("test2.jpg");
	if(image1 == 0 || image2 == 0)
	{
		printf("can not find test1.jpg and test2.jpg");
		exit(-1);
	}

	//ROIBlend* roiBlend = new ROIBlend(0.9, 2.0); // dummy parameters
	ROIBlend* roiBlend = new ROIBlend();

	//IplImage* image = roiBlend->BlendImages2(image1, image2, 20, 20);
	
	cvNamedWindow("Test");
	int size = 80;
	int std = 20;
	cvCreateTrackbar("Size", "Test", &size, 100, NULL);
	cvCreateTrackbar("Std", "Test", &std, 100, NULL);
	while(1)
	{
		IplImage* image = roiBlend->BlendImages3(image1, image2, size, std);
		cvShowImage("Test", image);
		cvWaitKey(100);
	}
}
int TestROIBlend( int argc, char** argv )
{	
	if(argc>1)
	{
		IplImage *src;/*, *dst, *src1;*/	
		ROIBlend* roiBlend = new ROIBlend(2.0, 2.0);

		src=cvLoadImage(argv[0],1);
		/*dst=cvLoadImage(argv[2],1);
		src1=cvLoadImage(argv[3],1);*/

		roiBlend->cvBlendImages(src);
		IplImage* src1 = cvLoadImage(argv[1], 1);;
		roiBlend->cvBlendImages(src1);
		
		IplImage* src2 = cvLoadImage(argv[2],1);
		roiBlend->cvBlendImages(src2);
		/*cvBlendImages(dst);
		cvBlendImages(src1);*/
		roiBlend->cvShowManyImages("Images", 3, src, src1, src2);	
		
		cvWaitKey();

		cvReleaseImage(&src);
		/*cvReleaseImage(&dst);
		cvReleaseImage(&src1);*/
	}

	return 0;
}
//Commented for later use in 2D
 
/*void cvShowManyImages(char* title, int nArgs, ...) {

    // img - Used for getting the arguments 
    IplImage *img;
	CvPoint p1, p2;

    // DispImage - the image in which input images are to be copied
    IplImage *DispImage;

    int size;
    int i;
    int m, n;
    int x, y;

    // w - Maximum number of images in a row 
    // h - Maximum number of images in a column 
    int w, h;

    // scale - How much we have to resize the image
    float scale;
    int max;

    // If the number of arguments is lesser than 0 or greater than 12
    // return without displaying 
    if(nArgs <= 0) {
        printf("Number of arguments too small....\n");
        return;
    }
    else if(nArgs > 12) {
        printf("Number of arguments too large....\n");
        return;
    }
    // Determine the size of the image, 
    // and the number of rows/cols 
    // from number of arguments 
    else if (nArgs == 1) {
        w = h = 1;
        size = 300;
    }
    else if (nArgs == 2) {
        w = 2; h = 1;
        size = 300;
    }
    else if (nArgs == 3 || nArgs == 4) {
        w = 2; h = 2;
        size = 300;
    }
    else if (nArgs == 5 || nArgs == 6) {
        w = 3; h = 2;
        size = 200;
    }
    else if (nArgs == 7 || nArgs == 8) {
        w = 4; h = 2;
        size = 200;
    }
    else {
        w = 4; h = 3;
        size = 150;
    }
	
    // Create a new 3 channel image
    DispImage = cvCreateImage( cvSize(size*w, size*h), 8, 3 );

    // Used to get the arguments passed
    va_list args;
    va_start(args, nArgs);

    // Loop for nArgs number of arguments
    for (i = 0, m = 20, n = 20; i < nArgs; i++, m += (size)-50) {

        // Get the Pointer to the IplImage
        img = va_arg(args, IplImage*);

        // Check whether it is NULL or not
        // If it is NULL, release the image, and return
        if(img == 0) {
            printf("Invalid arguments");
            cvReleaseImage(&DispImage);
            return;
        }

        // Find the width and height of the image
        x = img->width;
        y = img->height;

        // Find whether height or width is greater in order to resize the image
        max = (x > y)? x: y;

        // Find the scaling factor to resize the image
        scale = (float) ( (float) max / size );

        // Used to Align the images
        if( i % w == 0 && m!= 20) {
            //m = 20;
            //n+= 20 + size;
			m = 20;
            n+= size-150;
        }
		
        // Set the image ROI to display the current image
        cvSetImageROI(DispImage, cvRect(m, n, (int)( x/scale ), (int)( y/scale )));
		
        // Resize the input image and copy it to the Single Big Image
        cvResize(img, DispImage);

        // Reset the ROI in order to display the next image
        cvResetImageROI(DispImage);
		p1.x = (int)( x/scale );
		p1.y = (int)( y/scale );
		p2.x = m;
		p2.y = n;
		cvRectangle(DispImage, p1, p2, CV_RGB(255,0,0), 3, 8, 0 );

		printf("p1x=%d\np1y=%d\np2x=%d\np2y=%d\n", p1.x, p1.y, p2.x, p2.y);	
    }		 

    // Create a new window, and show the Single Big Image
    cvNamedWindow( title, 1 );
    cvShowImage( title, DispImage);

    cvWaitKey();
    cvDestroyWindow(title);

    // End the number of arguments
    va_end(args);

    // Release the Image Memory
    cvReleaseImage(&DispImage);	
}
*/


//void cvShowManyImages(char* title, int nArgs, ...) {
//
//    // img - Used for getting the arguments 
//    IplImage *img;
//	CvPoint p1, p2, p3;
//
//    // DispImage - the image in which input images are to be copied
//    IplImage *DispImage;
//
//    int size;
//    int i;
//    int m, n;
//    int x, y;
//
//    // w - Maximum number of images in a row 
//    // h - Maximum number of images in a column 
//    int w, h;
//
//    // scale - How much we have to resize the image
//    float scale;
//    int max;
//
//    // If the number of arguments is lesser than 0 or greater than 12
//    // return without displaying 
//    if(nArgs <= 0) {
//        printf("Number of arguments too small....\n");
//        return;
//    }
//    else if(nArgs > 12) {
//        printf("Number of arguments too large....\n");
//        return;
//    }
//    // Determine the size of the image, 
//    // and the number of rows/cols 
//    // from number of arguments 
//    else if (nArgs == 1) {
//        w = h = 1;
//        size = 300;
//    }
//    else if (nArgs == 2) {
//        w = 2; h = 1;
//        size = 300;
//    }
//    else if (nArgs == 3 || nArgs == 4) {
//        w = 3; h = 1;
//        size = 300;
//    }
//    else if (nArgs == 5 || nArgs == 6) {
//        w = 3; h = 2;
//        size = 200;
//    }
//    else if (nArgs == 7 || nArgs == 8) {
//        w = 4; h = 2;
//        size = 200;
//    }
//    else {
//        w = 4; h = 3;
//        size = 150;
//    }  
//	
//    // Create a new 3 channel image
//    DispImage = cvCreateImage( cvSize(size*w, size*h), 8, 3 );
//
//    // Used to get the arguments passed
//    va_list args;
//    va_start(args, nArgs);
//
//    // Loop for nArgs number of arguments
//    for (i = 0, m = 20, n = 20; i < nArgs; i++, m += (size)) {
//
//        // Get the Pointer to the IplImage
//        img = va_arg(args, IplImage*);
//
//        // Check whether it is NULL or not
//        // If it is NULL, release the image, and return
//        if(img == 0) {
//            printf("Invalid arguments");
//            cvReleaseImage(&DispImage);
//            return;
//        }
//
//        // Find the width and height of the image
//        x = img->width;
//        y = img->height;
//
//        // Find whether height or width is greater in order to resize the image
//        max = (x > y)? x: y;
//
//        // Find the scaling factor to resize the image
//        scale = (float) ( (float) max / size );
//
//        // Used to Align the images
//        if( i % w == 0 && m!= 20) {
//            //m = 20;
//            //n+= 20 + size;
//			m = 20;
//			n += y;
//        }		
//		
//        // Set the image ROI to display the current image
//        cvSetImageROI(DispImage, cvRect(m, n, (int)( x/scale ), (int)( y/scale )));
//		
//        // Resize the input image and copy it to the Single Big Image
//        cvResize(img, DispImage);
//
//        // Reset the ROI in order to display the next image
//        cvResetImageROI(DispImage);						
//
//		/*p1.x = m;
//		p1.y = n;
//		p2.x = m;
//		p2.y = (int)( y/scale )+20;
//		
//
//		cvLine(DispImage, p1, p2, CV_RGB(0,0,255), 2, 8, 0);*/
//
//		p1.x = m-20;
//		p1.y = n;
//		p2.x = m;
//		p2.y = (int)( y/scale )+20;
//		p3.x = m+20;
//		p3.y = n;
//		
//
//		cvRectangle(DispImage, p1, p2, CV_RGB(0,0,255), 2, 8, 0);
//		cvRectangle(DispImage, p3, p2, CV_RGB(255,0,0), 2, 8, 0);
//
//		/*printf("p1x=%d\np1y=%d\np2x=%d\np2y=%d\n", p1.x, p1.y, p2.x, p2.y);*/	
//    }		 		
//
//    // Create a new window, and show the Single Big Image
//    cvNamedWindow( title, 1 );
//    cvShowImage( title, DispImage);
//
//    cvWaitKey();
//    cvDestroyWindow(title);
//
//    // End the number of arguments
//    va_end(args);
//
//    // Release the Image Memory
//    cvReleaseImage(&DispImage);	
//}
//
//void calcFsofP(int val, int variance, char chan)
//{
//	float den=0;	
//	double pi=3.1415926, fs;
//
//	den=sqrt(2*pi*variance);
//	fs=exp((double)(val/den));
//
//	if(chan=='B')
//		printf("%c %f\n", chan, fs);
//	else
//		printf("%c %f ", chan, fs);
//}
//
//void cvBlendImages(IplImage *blend1)
//{
//	int width, height, rmean, gmean, bmean, rt=0, bt=0, gt=0, clw=0, rvar=0, gvar=0, bvar=0;
//	long rvartot=0, gvartot=0, bvartot=0;
//	width = blend1->width;
//	height = blend1->height;	
//
//	CvScalar s;
//	for(int i=0;i<height;i++){
//		for(int j=width-20;j<width;j++){
//			s = cvGet2D(blend1, i, j);
//			rt += s.val[2];
//			gt += s.val[1];
//			bt += s.val[0];
//			clw++;
//			//printf("B=%f, G=%f, R=%f\n",s.val[0],s.val[1],s.val[2]);			
//		}		
//	}
//	rmean = rt/clw;
//	bmean = bt/clw;
//	gmean = gt/clw;
//
//	//printf("R mean=%d\nG mean=%d\nB mean=%d\n",rmean, gmean, bmean);
//
//	for(int i=0;i<height;i++)
//	{
//		for(int j=width-20; j<width;j++)
//		{
//			s = cvGet2D(blend1, i, j);
//			rvartot += pow((s.val[2]-rmean),2);				
//			gvartot += pow((s.val[1]-rmean),2);			
//			bvartot += pow((s.val[0]-rmean),2);			
//		}
//	}
//	rvar = rvartot/clw;
//	gvar = gvartot/clw;
//	bvar = bvartot/clw;
//
//	//printf("R var=%d\nG var=%d\nB var=%d\nTotal=%d\n",rvartot, gvartot, bvartot, clw);
//
//	float r, g, b, rval, gval, bval;
//	rval=gval=bval=0;
//	for(int i=0;i<height;i++)
//	{
//		for(int j=width-20; j<width;j++)
//		{
//			s = cvGet2D(blend1, i, j);
//			r = s.val[2];
//			g = s.val[1];
//			b = s.val[0];
//						
//			rval = pow((double)(r - rmean),2)/(2*rvar);			
//			gval = pow((double)(g - gmean),2)/(2*gvar);
//			bval = pow((double)(b - bmean),2)/(2*bvar);
//
//			calcFsofP(rval, rvar, 'R');
//			calcFsofP(gval, gvar, 'G');
//			calcFsofP(bval, bvar, 'B');
//		}
//	}
//
//	//printf("E r=%d\nE g=%d\nE b=%d\n",rval, gval, bval);
//}
//
//


// Main function, defines the entry point for the program.
