#ifdef _WIN32
	#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// OpenGL & GLUT
#ifndef __APPLE__
	#include <GL/gl.h>
	#include <GL/glut.h>
#else
	#include <OpenGL/gl.h>
	#include <GLUT/glut.h>
#endif

// ARToolkit
#include <AR/gsub.h>
#include <AR/video.h>
#include <AR/param.h>
#include <AR/ar.h>

#include "FileWriter.h"
#include "ImageLoader.h"

// Global variables
ARUint8*		dataPtr;
int             img_width, img_height; // With and height of loaded image
int             thresh = 100;

char           *cparam_name = "Data/camera_para.dat";
ARParam         cparam;

/*
	Initialize ARToolkit
*/
static void init()
{
	ARParam  wparam;

	// set the initial camera parameters
	if (arParamLoad(cparam_name, 1, &wparam) < 0) 
	{
		printf("Camera parameter load error !!\n");
		exit(0);
	}

	arParamChangeSize(&wparam, img_width, img_height, &cparam);
	arInitCparam(&cparam);

	argInit(&cparam, 1.0, 0, 0, 0, 0); // open the graphics window
}

/*
	Helper functions, ideall we should move these to another file instead of main.c
*/

double get_dist(double* coord_1, double* coord_2){
	double result = sqrt(pow((coord_2[1] - coord_1[1]), 2) + pow((coord_2[0] - coord_2[1]), 2));
	return  result;
}

void get_pair(double** pair, ARMarkerInfo* marker_info, int marker_num){
	double ab = get_dist(marker_info[0].pos, marker_info[1].pos);
	double ac = get_dist(marker_info[0].pos, marker_info[2].pos);
	double bc = get_dist(marker_info[1].pos, marker_info[2].pos);
	double max = max(max(ab, ac), bc);
	if (max == ab){
		pair[0] = marker_info[0].pos;
		pair[1] = marker_info[1].pos;
	}
	else if (max == ac){
		pair[0] = marker_info[0].pos;
		pair[1] = marker_info[2].pos;

	}
	else if (max == bc){
		pair[0] = marker_info[1].pos;
		pair[1] = marker_info[2].pos;

	}
}

void get_midpoint(double* midpoint, double** pair){
	midpoint[0] = (max(pair[0][0], pair[1][0]) + min(pair[0][0], pair[1][0])) / 2.0;
	midpoint[1] = (max(pair[0][1], pair[1][1]) + min(pair[0][1], pair[1][1])) / 2.0;
}

/*
	Main loop for ARToolkit. Runs until program exits
	img_name: The name of the image file. This is used when writing results to csv file
*/

static void mainLoop(char* img_name)
{
	ARMarkerInfo    *marker_info;
	int             marker_num;

	argDrawMode2D();

	// detect the markers in the video frame
	if (arDetectMarker(dataPtr, thresh, &marker_info, &marker_num) < 0) 
	{		
		exit(0); // Quit if marker detection failed
	}
	printf("%i patterns found\n", marker_num);
	// If you uncomment the following code the program will run until it detects at least 1 marker.
	// It will then print out the pattern locations and exit
	if (marker_num > 2)
	{
		//printf("Found %i markers:\n", marker_num);
		/*for (int i = 0; i < marker_num; i++) // Print position of each marker
		{	
			printf("marker %i: %f, %f\n", i, marker_info[i].pos[0], marker_info[i].pos[1]);
		}*/
		double midpoint[2];
		double pair[2][2];
		get_pair(&pair, marker_info, marker_num);
		get_midpoint(&midpoint, &pair);
		printf("Center is: %5i,%5i\n", (int)midpoint[0], (int)midpoint[1]);
		writeLine(img_name, (int)midpoint[0], (int)midpoint[1]);
	}
	else
	{
		printf("%i patterns isnt enough to find a center point!\n", marker_num);
		writeLine(img_name, 0, 0);
	}
	//argSwapBuffers();
}

int main(int argc, char **argv)
{
	if (argc == 1) // If no images were provided
	{
		printf("USAGE: AR_Simple.exe img1.jpg img2.jpg ...\n");
		exit(1);
	}

	for (int i = 1; i < argc; i++) // Check that each arg is a .jpg image
	{
		if (strstr(argv[i], ".jpg") == NULL)
		{
			printf("Images must be .jpg format. %s is not a valid file\n", argv[i]);
			exit(1);
		}
	}

	dataPtr = loadImage(argv[1], dataPtr, &img_width, &img_height); // Load the first image (so init() can properly register the image size)
	if (dataPtr == NULL) // If image failed to load
		exit(1);

	glutInit(&argc, argv);

	createOutputFile(); // Delete any old output.csv file and create a fresh one
	for (int i = 1; i < argc; i++) // For each input image
	{
		dataPtr = loadImage(argv[i], dataPtr, &img_width, &img_height);
		if (dataPtr == NULL)
		{
			printf("Failed to load image: '%s' It will be skipped...\n", argv[i]);
			continue;
		}
		init(); // Initialize ARToolkit for this size of image
		printf("Running on %s:\n", argv[i]);
		mainLoop(argv[i]); // Detect sphere location on this image
		free(dataPtr);
		printf("=================== Done! ===================\n\n");
	}
	printf("============================\n");
	printf("See output.csv for results\n");
	printf("============================\n");
	return 0;
}
