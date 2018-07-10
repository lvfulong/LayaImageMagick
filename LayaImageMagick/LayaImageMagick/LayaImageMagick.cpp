// LayaImageMagick.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <Magick++.h>
#include <string>
#include <iostream>
#include <cmath>

using namespace std;
/*
argv[1] 输入文件 绝对路径
argv[2] 输出文件 绝对路径
*/

// 把HDR的Float類型轉成RGBE類型的像素
void FloatRGB2RGBE(int InImageSize, Magick::Quantum* InPixelBuffer, unsigned char* OutRGBEBuffer)
{

    for (int i = 0; i < InImageSize; i++)
    {
        Magick::Quantum red = InPixelBuffer[i * 4 + 0];
        Magick::Quantum green = InPixelBuffer[i * 4 + 1];
        Magick::Quantum blue = InPixelBuffer[i * 4 + 2];

        double maxValue = red;
        if (green > maxValue) maxValue = green;
        if (blue > maxValue) maxValue = blue;

        if (maxValue < std::numeric_limits<Magick::Quantum>::epsilon())
        {
            OutRGBEBuffer[i * 4 + 0] = OutRGBEBuffer[i * 4 + 1]
                = OutRGBEBuffer[i * 4 + 2] = OutRGBEBuffer[i * 4 + 3] = 0;
        }
        else
        {
            int exponent = 0;
            double mantissa = frexp(maxValue, &exponent);
            if (mantissa < 0)
            {
                OutRGBEBuffer[i * 4 + 0] = OutRGBEBuffer[i * 4 + 1]
                    = OutRGBEBuffer[i * 4 + 2] = OutRGBEBuffer[i * 4 + 3] = 0;
                return;
            }
            else
            {
                // 当某个值为v的时候，其尾数就是e[0]。 这里*256了，所以反向的时候有个/256即-(128+8)里的8
                // e[0]永远不会为1所以结果<256
                double scaleRatio = (double)(mantissa * 256.0f / maxValue);
                OutRGBEBuffer[i * 4 + 0] = (unsigned char)(red * scaleRatio);
                OutRGBEBuffer[i * 4 + 1] = (unsigned char)(green * scaleRatio);
                OutRGBEBuffer[i * 4 + 2] = (unsigned char)(blue * scaleRatio);
                OutRGBEBuffer[i * 4 + 3] = (unsigned char)(exponent + 128);
            }
        }
    }

}
int main(int argc, char **argv)
{
 
    if (argc < 3)
    {
        cout << "Lack parameter"<< endl;
        return 1;
    }
    Magick::InitializeMagick(*argv);
    
    try {
        Magick::Image image;
        image.read(argv[1]);
        //image.read("E:\\laya\\LayaImageMagick\\LayaImageMagick\\Release\\Lightmap-0_comp_light.exr");
        //image.type(Magick::ImageType::TrueColorType);
        //image.modifyImage();
        size_t columns = image.columns();
        size_t rows = image.rows();
        //image.magick("RGBA");
        Magick::Quantum *pixels = image.getPixels(0, 0, columns, rows);
        //Magick::Pixels view(image);
        //Magick::Quantum *pixels = view.get(0, 0, columns, rows);
        Magick::Quantum* fixedPixelBuffer = new  Magick::Quantum[columns * rows * 4];
        unsigned char* rgbeBuffer = new unsigned char[columns * rows * 4];
        ssize_t i = 0;
        for (ssize_t r = 0; r < rows; ++r)
        {
            for (ssize_t c = 0; c < columns; ++c)
            {
                fixedPixelBuffer[i++] = pixels[(r * columns + c) * 4 + 0] / QuantumRange;
                fixedPixelBuffer[i++] = pixels[(r * columns + c) * 4 + 1] / QuantumRange;
                fixedPixelBuffer[i++] = pixels[(r * columns + c) * 4 + 2] / QuantumRange;
                fixedPixelBuffer[i++] = pixels[(r * columns + c) * 4 + 3] / QuantumRange;
            }
        }
        //把HDR的Float類型轉成RGBE類型的像素
        FloatRGB2RGBE(columns * rows, fixedPixelBuffer, rgbeBuffer);
        Magick::Image imageOut(columns, rows, "RGBA", Magick::StorageType::CharPixel, rgbeBuffer);
        //view.sync();
        //imageOut.magick("PNG");
        //imageOut.write("E:\\laya\\LayaImageMagick\\LayaImageMagick\\Release\\Lightmap-0_comp_light.png");
        //imageOut.write(argv[2]);

        FILE *fp = fopen(argv[2],"wb");
        if (fp == NULL)
        {
            cout << "fopen error" << endl;
            fclose(fp);
            return 1;
        }

        if (fwrite(&columns, sizeof(int), 1, fp) != 1)
        {
            cout << "fwrite columns error" << endl;
            fclose(fp);
            return 1;
        }
        if (fwrite(&rows, sizeof(int), 1, fp) != 1)
        {
            cout << "fwrite rows error" << endl;
            fclose(fp);
            return 1;
        }
        if (fwrite(rgbeBuffer, columns * rows * 4, 1, fp) != 1)
        {
            cout << "fwrite rgbeBuffer error" << endl;
            fclose(fp);
            return 1;
        }
        fclose(fp);
    }
    catch (Magick::Exception &error_)
    {
        cout << "Caught exception: " << error_.what() << endl;
        Magick::TerminateMagick();
        return 1;
    }
    Magick::TerminateMagick();
    return 0;
}

