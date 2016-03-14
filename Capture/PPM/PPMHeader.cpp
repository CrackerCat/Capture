#include "PPMHeader.h"

#include<sstream>

std::string printf_img(unsigned w, unsigned h, char * img)
{
	std::stringstream ppmstream;

	ppmstream << "P6" << std::endl;
	ppmstream << w << ' ';
	ppmstream << h << std::endl;
	ppmstream << 255 << std::endl;

	ppmstream.write(img, w*h * 3);

	return ppmstream.str();
}
