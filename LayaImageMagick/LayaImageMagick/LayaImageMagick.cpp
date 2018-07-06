// LayaImageMagick.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <Magick++.h>
#include <string>
#include <iostream>

using namespace std;
/*
argv[1] �����ļ� ����·��
argv[2] ����ļ� ����·��
*/

struct Fresp
{
    Fresp(bool negative, int exponent, float mantissa)
        :negative(negative),
        exponent(exponent),
        mantissa(mantissa)
    {}
    bool negative = false;
    int exponent = 0;
    float mantissa = 0.0f;
};
// ��һ�����c���D�Q��X*2^N����ʽ 
// ����8.000000 = 0.500000 * 2^4��123.45 = 0.964453 * 2^7
Fresp SHARP_frexp(double d)
{
    // Translate the double into sign, exponent and mantissa.
    int64_t bits = (int64_t)d;//BitConverter.DoubleToInt64Bits(d);
    // Note that the shift is sign-extended, hence the test against -1 not 1
    bool negative = (bits < 0);
    int exponent = (int)((bits >> 52) & 0x7ffL);
    int64_t mantissa = bits & 0xfffffffffffffL;

    // Subnormal numbers; exponent is effectively one higher,
    // but there's no extra normalisation bit in the mantissa
    if (exponent == 0)
    {
        exponent++;
    }
    // Normal numbers; leave exponent as it is but add extra
    // bit to the front of the mantissa
    else
    {
        mantissa = mantissa | (1L << 52);
    }

    // Bias the exponent. It's actually biased by 1023, but we're
    // treating the mantissa as m.0 rather than 0.m, so we need
    // to subtract another 52 from it.
    exponent -= 1075;

    if (mantissa == 0)
    {
        return Fresp(false, 0, 0.0f);
    }

    /* Normalize */
    while ((mantissa & 1) == 0)
    {    /*  i.e., Mantissa is even */
        mantissa >>= 1;
        exponent++;
    }

    // ԭ����㷨��m.0*2^n Ҫ�Q��0.m*2^n
    float floatMantissa = mantissa;
    while (floatMantissa > 1.0f - std::numeric_limits<float>::epsilon())
    {
        floatMantissa /= 2.0f;
        exponent++;
    }
    return Fresp(negative, exponent, floatMantissa);
}
// ��HDR��Float����D��RGBE��͵�����
void FloatRGB2RGBE(int InImageSize, Magick::Quantum* InPixelBuffer, char* OutRGBEBuffer)
{

    for (int i = 0; i < InImageSize; i++)
    {
        Magick::Quantum red = InPixelBuffer[i * 4 + 0];
        Magick::Quantum green = InPixelBuffer[i * 4 + 1];
        Magick::Quantum blue = InPixelBuffer[i * 4 + 2];

        Magick::Quantum maxValue = red;
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
            float mantissa = 0;
            Fresp tuple = SHARP_frexp(maxValue);
            if (tuple.negative)
            {
                OutRGBEBuffer[i * 4 + 0] = OutRGBEBuffer[i * 4 + 1]
                    = OutRGBEBuffer[i * 4 + 2] = OutRGBEBuffer[i * 4 + 3] = 0;
                return;
            }
            else
            {
                exponent = tuple.exponent;
                mantissa = tuple.mantissa;
                // ��ĳ��ֵΪv��ʱ����β������e[0]�� ����*256�ˣ����Է����ʱ���и�/256��-(128+8)���8
                // e[0]��Զ����Ϊ1���Խ��<256
                Magick::Quantum scaleRatio = (Magick::Quantum)(mantissa * 256.0f / maxValue);
                OutRGBEBuffer[i * 4 + 0] = (char)(red * scaleRatio);
                OutRGBEBuffer[i * 4 + 1] = (char)(green * scaleRatio);
                OutRGBEBuffer[i * 4 + 2] = (char)(blue * scaleRatio);
                OutRGBEBuffer[i * 4 + 3] = (char)(exponent + 128);
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
        char* rgbeBuffer = new char[columns * rows * 4];
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
        //��HDR��Float����D��RGBE��͵�����
        FloatRGB2RGBE(columns * rows, fixedPixelBuffer, rgbeBuffer);
        Magick::Image imageOut(columns, rows, "RGBA", Magick::StorageType::CharPixel, rgbeBuffer);
        //view.sync();
        //imageOut.magick("PNG");
        //imageOut.write("E:\\laya\\LayaImageMagick\\LayaImageMagick\\Release\\Lightmap-0_comp_light.png");
        imageOut.write(argv[2]);
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

