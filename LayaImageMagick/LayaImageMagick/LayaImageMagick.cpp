// LayaImageMagick.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <Magick++.h>
#include <string>
#include <iostream>

using namespace std;

using namespace Magick;
/*
argv[1] 输入文件 绝对路径
argv[2] 输出文件 绝对路径
*/
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cout << "Lack parameter"<< endl;
        return 1;
    }
    // Construct the image object. Seperating image construction from the 
    // the read operation ensures that a failure to read the image file 
    // doesn't render the image object useless. 
    Image image;
   
    try {
        // Read a file into image object 
        image.read(argv[1]);

        // Write to BLOB in RGBA format 
        Blob blob;
        // Set RGBA output format 
        image.magick("RGBA");
        image.write(&blob);

        image.magick("PNG");
        image.write(argv[2]);
    }
    catch (Exception &error_)
    {
        cout << "Caught exception: " << error_.what() << endl;
        return 1;
    }
    return 0;
}

