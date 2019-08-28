#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>

#include <fstream>
#include <iostream>
#include <map>


#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

using namespace std;
using namespace cv;

class Annotations {
public:
	vector<Rect> annotations;
	vector<string> annotationsLabels;
	int size;
	Annotations() {
		size = 0;
	}
};

// from here: https://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c?rq=1
vector<string> get_all_files_names_within_folder(string folder, string firstFileName)
{
	vector<string> names;
	string search_path = folder + "*.*";
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			// read all (real) files in current folder
			// , delete '!' read other 2 default folder . and ..
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				if( firstFileName.compare( fd.cFileName) != 0)
					names.push_back(fd.cFileName);
			}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}
	return names;
}

// Function prototypes
void on_mouse(int, int, int, int, void*);
Annotations get_annotations(Mat);

// Public parameters
Mat image;
int roi_x0 = 0, roi_y0 = 0, roi_x1 = 0, roi_y1 = 0, num_of_rec = 0;
bool start_draw = false, stop = false;

// Window name for visualisation purposes
string window_name = "AnnotationTool";

string labels[10] = { "n/a","n/a","n/a","n/a","n/a","n/a","n/a","n/a","n/a","n/a" };
string output_folder = "";
string firstFileName = "";
int gotoNextImage = 0;
int sequential = 1;
vector<string> allFiles;
int fileCounter = 0;
int processedImages = 0;

//modified from here:
//https://github.com/opencv/opencv/blob/master/apps/annotation/opencv_annotation.cpp
// FUNCTION : Mouse response for selecting objects in images
// If left button is clicked, start drawing a rectangle as long as mouse moves
// Stop drawing once a new left click is detected by the on_mouse function
void on_mouse(int event, int x, int y, int, void *)
{
	// Action when left button is clicked
	if (event == EVENT_LBUTTONDOWN)
	{
		if (!start_draw)
		{
			roi_x0 = x;
			roi_y0 = y;
			start_draw = true;
		}
		else {
			roi_x1 = x;
			roi_y1 = y;
			start_draw = false;
		}
	}

	// Action when mouse is moving and drawing is enabled
	if ((event == EVENT_MOUSEMOVE) && start_draw)
	{
		// Redraw bounding box for annotation
		Mat current_view;
		image.copyTo(current_view);
		rectangle(current_view, Point(roi_x0, roi_y0), Point(x, y), Scalar(0, 0, 255));
		imshow(window_name, current_view);
	}
}

// FUNCTION : returns a vector of Rect objects given an image containing positive object instances
Annotations get_annotations(Mat input_image)
{
	Annotations currentAnnotations;
	//vector<Rect> current_annotations;
	//vector<string> current_annotation_labels;

	// Make it possible to exit the annotation process
	stop = false;

	// Init window interface and couple mouse actions
	namedWindow(window_name, WINDOW_AUTOSIZE);
	moveWindow(window_name, 50, 50);
	setMouseCallback(window_name, on_mouse);
	image = input_image;
	imshow(window_name, image);
	int key_pressed = 0;
	gotoNextImage = 0;

	do
	{
		// Get a temporary image clone
		Mat temp_image = input_image.clone();
		Rect currentRect(0, 0, 0, 0);

		// Keys for processing
		// You need to select one for confirming a selection and one to continue to the next image
		// Based on the universal ASCII code of the keystroke: http://www.asciitable.com/
		//     0,1,2,3,4,5,6,7,8,9 (48-54)		    add rectangle to current image with specified classid
		//	    n = 110		    save added rectangles and show next image
		//      d = 100         delete the last annotation made
		//	    <ESC> = 27      exit program
		key_pressed = 0xFF & waitKey(0);
		if (key_pressed == 27) {
			stop = true;
		}
		else if (key_pressed >= 48 && key_pressed <= 57) {
			int classid = (key_pressed - 48);
			//if there is a class label for the pressed number
			if (labels[classid].compare("n/a") != 0) {
				// Draw initiated from top left corner
				if (roi_x0 < roi_x1 && roi_y0 < roi_y1)
				{
					currentRect.x = roi_x0;
					currentRect.y = roi_y0;
					currentRect.width = roi_x1 - roi_x0;
					currentRect.height = roi_y1 - roi_y0;
				}
				// Draw initiated from bottom right corner
				if (roi_x0 > roi_x1 && roi_y0 > roi_y1)
				{
					currentRect.x = roi_x1;
					currentRect.y = roi_y1;
					currentRect.width = roi_x0 - roi_x1;
					currentRect.height = roi_y0 - roi_y1;
				}
				// Draw initiated from top right corner
				if (roi_x0 > roi_x1 && roi_y0 < roi_y1)
				{
					currentRect.x = roi_x1;
					currentRect.y = roi_y0;
					currentRect.width = roi_x0 - roi_x1;
					currentRect.height = roi_y1 - roi_y0;
				}
				// Draw initiated from bottom left corner
				if (roi_x0<roi_x1 && roi_y0>roi_y1)
				{
					currentRect.x = roi_x0;
					currentRect.y = roi_y1;
					currentRect.width = roi_x1 - roi_x0;
					currentRect.height = roi_y0 - roi_y1;
				}
				// Draw the rectangle on the canvas
				// Add the rectangle to the vector of annotations
				currentAnnotations.annotations.push_back(currentRect);
				currentAnnotations.annotationsLabels.push_back(labels[classid]);
				currentAnnotations.size++;
			}
		}
		else if (key_pressed == 68 || key_pressed == 100) {
			// Remove the last annotation
			if (currentAnnotations.size > 0) {
				currentAnnotations.annotations.pop_back();
				currentAnnotations.annotationsLabels.pop_back();
				currentAnnotations.size--;
			}
		}
		else if (key_pressed == 87 || key_pressed == 119) {
			//dump the current annotation list to outputfolder
		}
		// Check if escape has been pressed
		if (stop)
		{
			break;
		}

		// Draw all the current rectangles onto the top image and make sure that the global image is linked
		for (int i = 0; i < currentAnnotations.size; i++) {
			rectangle(temp_image, currentAnnotations.annotations[i], Scalar(0, 255, 0), 1);
		}
		image = temp_image;

		// Force an explicit redraw of the canvas --> necessary to visualize delete correctly
		imshow(window_name, image);
	}
	// Continue as long as the next image key has not been pressed
	while (key_pressed != 110 && key_pressed != 32);

	if (key_pressed == 110 || key_pressed == 32) {
		gotoNextImage = 1;
	}

	// Close down the window
	destroyWindow(window_name);

	// Return the data
	return currentAnnotations;
}

string GetFileName(const string & prompt) {
	const int BUFSIZE = 1024;
	char buffer[BUFSIZE] = { 0 };
	OPENFILENAME ofns = { 0 };
	ofns.lStructSize = sizeof(ofns);
	ofns.lpstrFile = buffer;
	ofns.nMaxFile = BUFSIZE;
	ofns.lpstrTitle = prompt.c_str();
	GetOpenFileName(&ofns);
	return buffer;
}

// https://stackoverflow.com/questions/25829143/trim-whitespace-from-a-string/25829178
string trim(const string& str)
{
	size_t first = str.find_first_not_of(' ');
	if (string::npos == first)
	{
		return str;
	}
	size_t last = str.find_last_not_of(' ');
	return str.substr(first, (last - first + 1));
}

int readConfigFile() {

	ifstream input("config.txt");
	if (input) {
		for (string line; getline(input, line); ){
			//class label
			if (line.rfind("class") == 0 && line.find(":") != string::npos) {
				string::size_type loc1 = line.find("class", 0) + 5; //5 is from the length of class
				string::size_type loc2 = line.find(":", 0);

				//loc1 to loc2 is the classid
				if (loc2 > loc1) {
					string classId = line.substr(loc1, loc2 - loc1);
					//just in case if someone has classa : cat
					try {
						int cid = stoi(classId);
						if (cid >= 0 && cid < 10) {
							string label = line.substr(loc2+1, line.length() - loc2 + 1);
							labels[cid] = trim(label);
						}
					}
					catch (...) {}
				}
			}
			//output_folder: where to dumpt the outputs
			else if (line.rfind("output_folder") == 0 && line.find(":") != string::npos) {
				string::size_type loc1 = line.find(":", 0);
				string location = line.substr(loc1 + 1, line.length() - loc1);
				output_folder = trim(location);
			}
			else if (line.rfind("iteration_type") == 0 && line.find(":") != string::npos) {
				string::size_type loc1 = line.find(":", 0);
				string t = line.substr(loc1 + 1, line.length() - loc1);
				transform(t.begin(), t.end(), t.begin(), [](unsigned char c) { return std::tolower(c); });
				if (t.compare("sequential") != 0) {
					sequential = 0;
				}
			}
		}
		return 0;
	}
	else {
		return -1;
	}

}


string getNextImage(size_t last_slash, string filename, size_t found) {

	//size_t last_slash = (firstFileName.rfind('\\') + 1);
	//string filename = firstFileName.substr(last_slash, firstFileName.length() - last_slash);
	//size_t found = filename.find_last_of(".");
	if (found != string::npos && found > 0) {
		int count = 0;
		int i = (found - 1);
		while (i >= 0 && isdigit(filename.at(i))) {
			i--;
			count++;
		}
		if (count > 0) {
			string base = firstFileName.substr(0, last_slash + 1 + i);
			string extension = filename.substr(found, filename.length() - found);
			string number = filename.substr((i + 1), count);
			int numberi = stoi(number);
			string numbers = to_string(numberi);
			string newFileName = "";
			//this means filename is something like image1.png
			if (number.length() == numbers.length()) {
				newFileName = base + to_string(numberi + 1) + extension;
			}
			//this means filename is something like image001.png
			else {
				newFileName = base;
				numberi++;
				string numberis = to_string(numberi);
				for (int i = 0; i < (count - numberis.length()); i++) {
					newFileName += "0";
				}
				newFileName += numberis + extension;
			}
			return newFileName;
		}
	}
	return "";

}


int main(int argc, const char** argv)
{
	cout << "program started" << endl;
	int config = readConfigFile();
	if (config == -1) {
		MessageBox(
			NULL,
			"config.txt is not found. \nPlease create one with your custom configurations!",
			"Confirm Save As",
			MB_ICONEXCLAMATION);
		return 0;
	}
	
	firstFileName = GetFileName("Pick an image to annotate: ");
	do {
		image = imread(firstFileName);
		if (image.data != NULL) {
			window_name = firstFileName;
			Annotations annotations = get_annotations(image);
			size_t last_slash = (firstFileName.rfind('\\') + 1);
			string filename = firstFileName.substr(last_slash, firstFileName.length() - last_slash);
			size_t found = filename.find_last_of(".");
			if (sequential == 0 && processedImages == 0) {
				allFiles = get_all_files_names_within_folder(firstFileName.substr(0, last_slash), firstFileName);
			}
			if (annotations.size > 0) {
				ofstream fileOut;
				string outfilename = output_folder + "\\" + filename.substr(0, found) + ".txt";
				fileOut.open(outfilename);
				if (fileOut.is_open()) {
					for (int i = 0; i < annotations.size; i++) {
						float xmiddle = annotations.annotations[i].tl().x + (annotations.annotations[i].width / 2.0);
						xmiddle = xmiddle / image.cols;

						float ymiddle = annotations.annotations[i].tl().y + (annotations.annotations[i].height / 2.0);
						ymiddle = ymiddle / image.rows;

						float width = (float)annotations.annotations[i].width / (float)image.cols;
						float height = (float)annotations.annotations[i].height / (float)image.rows;

						fileOut << annotations.annotationsLabels[i] << "," << xmiddle << "," << ymiddle;
						fileOut << "," << width << "," << height << "\n";
						//fileout << "Writing this to a file.\n";
					}
				}
				fileOut.close();
			}//
			if (gotoNextImage == 1) {
				if( sequential == 1)
					firstFileName = getNextImage(last_slash, filename, found);
				else {
					firstFileName = allFiles[fileCounter];
					fileCounter++;
				}
			}
		}
		else {
			//if there is no images to iterate
			gotoNextImage = 0;
			MessageBox(
				NULL,
				"No next image to process!",
				"Confirm Save As",
				MB_ICONEXCLAMATION);
		}
	} while (gotoNextImage == 1);

	int a = 1;
	
	return 0;
}