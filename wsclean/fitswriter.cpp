#include "fitswriter.h"
#include "fitsreader.h"

#include "uvector.h"

#include <stdexcept>
#include <sstream>
#include <vector>

#include <cmath>
#include <cstdio>
#include <limits>
#include <iostream>

void FitsWriter::writeHeaders(fitsfile*& fptr, const std::string& filename, size_t nFreq, size_t nPol) const
{
	int status = 0;
	fits_create_file(&fptr, (std::string("!") + filename).c_str(), &status);
	checkStatus(status, filename);
	
	// append image HDU
	int bitPixInt = FLOAT_IMG;
	long naxes[4];
	naxes[0] = _width;
	naxes[1] = _height;
	naxes[2] = nFreq;
	naxes[3] = nPol;
	fits_create_img(fptr, bitPixInt, 4, naxes, &status);
	checkStatus(status, filename);
	float zero = 0, one = 1, equinox = 2000.0;
	fits_write_key(fptr, TFLOAT, "BSCALE", (void*) &one, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "BZERO", (void*) &zero, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TSTRING, "BUNIT", (void*) "JY/BEAM", "Units are in Jansky per beam", &status); checkStatus(status, filename);
	
	if(_hasBeam)
	{
		double
			majDeg = setNotFiniteToZero(_beamMajorAxisRad * 180.0 / M_PI),
			minDeg = setNotFiniteToZero(_beamMinorAxisRad * 180.0 / M_PI), 
			posAngle = setNotFiniteToZero(_beamPositionAngle * 180.0 / M_PI);
		fits_write_key(fptr, TDOUBLE, "BMAJ", (void*) &majDeg, "", &status); checkStatus(status, filename);
		fits_write_key(fptr, TDOUBLE, "BMIN", (void*) &minDeg, "", &status); checkStatus(status, filename);
		fits_write_key(fptr, TDOUBLE, "BPA", (void*) &posAngle, "", &status); checkStatus(status, filename);
	}
	
	fits_write_key(fptr, TFLOAT, "EQUINOX", (void*) &equinox, "J2000", &status); checkStatus(status, filename);
	fits_write_key(fptr, TSTRING, "BTYPE", (void*) "Intensity", "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TSTRING, "ORIGIN", (void*) _origin.c_str(), _originComment.c_str(), &status); checkStatus(status, filename);
	float phaseCentreRADeg = (_phaseCentreRA/M_PI)*180.0, phaseCentreDecDeg = (_phaseCentreDec/M_PI)*180.0;
	float
		centrePixelX = _pixelSizeX!=0.0 ? ((_width / 2.0)+1.0 + _phaseCentreDL/_pixelSizeX) : (_width / 2.0)+1.0,
		centrePixelY = _pixelSizeY!=0.0 ? ((_height / 2.0)+1.0 - _phaseCentreDM/_pixelSizeY) : (_height / 2.0)+1.0,
		stepXDeg = (-_pixelSizeX / M_PI)*180.0,
		stepYDeg = ( _pixelSizeY / M_PI)*180.0;
	fits_write_key(fptr, TSTRING, "CTYPE1", (void*) "RA---SIN", "Right ascension angle cosine", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "CRPIX1", (void*) &centrePixelX, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "CRVAL1", (void*) &phaseCentreRADeg, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "CDELT1", (void*) &stepXDeg, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TSTRING, "CUNIT1", (void*) "deg", "", &status); checkStatus(status, filename);
	
	fits_write_key(fptr, TSTRING, "CTYPE2", (void*) "DEC--SIN", "Declination angle cosine", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "CRPIX2", (void*) &centrePixelY, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "CRVAL2", (void*) &phaseCentreDecDeg, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "CDELT2", (void*) &stepYDeg, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TSTRING, "CUNIT2", (void*) "deg", "", &status); checkStatus(status, filename);

	fits_write_key(fptr, TSTRING, "CTYPE3", (void*) "FREQ", "Central frequency", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "CRPIX3", (void*) &one, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TDOUBLE, "CRVAL3", (void*) &_frequency, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TDOUBLE, "CDELT3", (void*) &_bandwidth, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TSTRING, "CUNIT3", (void*) "Hz", "", &status); checkStatus(status, filename);
	
	float pol;
	switch(_polarization)
	{
		case Polarization::StokesI: pol = 1.0; break;
		case Polarization::StokesQ: pol = 2.0; break;
		case Polarization::StokesU: pol = 3.0; break;
		case Polarization::StokesV: pol = 4.0; break;
		case Polarization::RR: pol = -1.0; break;
		case Polarization::LL: pol = -2.0; break;
		case Polarization::RL: pol = -3.0; break;
		case Polarization::LR: pol = -4.0; break;
		case Polarization::XX: pol = -5.0; break;
		case Polarization::YY: pol = -6.0; break; //yup, this is really the right value
		case Polarization::XY: pol = -7.0; break;
		case Polarization::YX: pol = -8.0; break;
	}
	fits_write_key(fptr, TSTRING, "CTYPE4", (void*) "STOKES", "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "CRPIX4", (void*) &one, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "CRVAL4", (void*) &pol, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TFLOAT, "CDELT4", (void*) &one, "", &status); checkStatus(status, filename);
	fits_write_key(fptr, TSTRING, "CUNIT4", (void*) "", "", &status); checkStatus(status, filename);
	
	// RESTFRQ ?
	fits_write_key(fptr, TSTRING, "SPECSYS", (void*) "TOPOCENT", "", &status); checkStatus(status, filename);
	
  int year, month, day, hour, min, sec, deciSec;
	julianDateToYMD(_dateObs + 2400000.5, year, month, day);
	MJDToHMS(_dateObs, hour, min, sec, deciSec);
	char dateStr[40];
  std::sprintf(dateStr, "%d-%02d-%02dT%02d:%02d:%02d.%01d", year, month, day, hour, min, sec, deciSec);
	fits_write_key(fptr, TSTRING, "DATE-OBS", (void*) dateStr, "", &status); checkStatus(status, filename);
	
	// Extra keywords
	for(std::map<std::string, std::string>::const_iterator i=_extraStringKeywords.begin(); i!=_extraStringKeywords.end(); ++i)
	{
		const char* name = i->first.c_str();
		char* valueStr = const_cast<char*>(i->second.c_str());
		fits_write_key(fptr, TSTRING, name, valueStr, "", &status);
		checkStatus(status, filename);
	}
	for(std::map<std::string, double>::const_iterator i=_extraNumKeywords.begin(); i!=_extraNumKeywords.end(); ++i)
	{
		const char* name = i->first.c_str();
		double value = setNotFiniteToZero(i->second);
		fits_write_key(fptr, TDOUBLE, name, (void*) &value, "", &status);
		checkStatus(status, filename);
	}
	
	// History
	std::ostringstream histStr;
	for(std::vector<std::string>::const_iterator i=_history.begin(); i!=_history.end(); ++i)
	{
		fits_write_history(fptr, i->c_str(), &status);
		checkStatus(status, filename);
	}
}

void FitsWriter::writeImage(fitsfile* fptr, const std::string& filename, const double* image) const
{
	long firstpixel[4];
	for(int i=0;i < 4;i++) firstpixel[i] = 1;
	double nullValue = std::numeric_limits<double>::max();
	int status = 0;
	fits_write_pixnull(fptr, TDOUBLE, firstpixel, _width*_height, const_cast<double*>(image), &nullValue, &status);
	checkStatus(status, filename);
}

void FitsWriter::writeImage(fitsfile* fptr, const std::string& filename, const float* image) const
{
	long firstpixel[4];
	for(int i=0;i < 4;i++) firstpixel[i] = 1;
	float nullValue = std::numeric_limits<float>::max();
	int status = 0;
	fits_write_pixnull(fptr, TFLOAT, firstpixel, _width*_height, const_cast<float*>(image), &nullValue, &status);
	checkStatus(status, filename);
}

template<typename NumType>
void FitsWriter::writeImage(fitsfile* fptr, const std::string& filename, const NumType* image) const
{
	long firstpixel[4];
	for(int i=0;i < 4;i++) firstpixel[i] = 1;
	double nullValue = std::numeric_limits<double>::max();
	int status = 0;
	size_t totalSize = _width*_height;
	std::vector<double> copy(totalSize);
	for(size_t i=0;i!=totalSize;++i) copy[i] = image[i];
	fits_write_pixnull(fptr, TDOUBLE, firstpixel, totalSize, &copy[0], &nullValue, &status);
	checkStatus(status, filename);
}

void FitsWriter::WriteMask(const std::string& filename, const bool* mask) const
{
	ao::uvector<float> maskAsImage(_width * _height);
	for(size_t i=0; i!=_width*_height; ++i)
		maskAsImage[i] = mask[i] ? 1.0 : 0.0;
	Write(filename, maskAsImage.data());
}

template<typename NumType>
void FitsWriter::Write(const std::string& filename, const NumType* image) const
{
	fitsfile *fptr;

	writeHeaders(fptr, filename, 1, 1);
	
	writeImage(fptr, filename, image);
	
	int status = 0;
	fits_close_file(fptr, &status);
	checkStatus(status, filename);
}

template void FitsWriter::Write<long double>(const std::string& filename, const long double* image) const;
template void FitsWriter::Write<double>(const std::string& filename, const double* image) const;
template void FitsWriter::Write<float>(const std::string& filename, const float* image) const;

void FitsWriter::StartMulti(const std::string& filename, size_t nPol, size_t nFreq)
{
	if(_multiFPtr != 0)
		throw std::runtime_error("StartMulti() called twice without calling FinishMulti()");
	_multiFilename = filename;
	writeHeaders(_multiFPtr, _multiFilename, 1, 1);
}

void FitsWriter::FinishMulti()
{
	int status = 0;
	fits_close_file(_multiFPtr, &status);
	checkStatus(status, _multiFilename);
	_multiFPtr = 0;
}

void FitsWriter::SetMetadata(const FitsReader& reader)
{
	_width = reader.ImageWidth();
	_height = reader.ImageHeight();
	_phaseCentreRA = reader.PhaseCentreRA();
	_phaseCentreDec = reader.PhaseCentreDec();
	_pixelSizeX = reader.PixelSizeX();
	_pixelSizeY = reader.PixelSizeY();
	_frequency = reader.Frequency();
	_bandwidth = reader.Bandwidth();
	_dateObs = reader.DateObs();
	_polarization = reader.Polarization();
	_hasBeam = reader.HasBeam();
	if(_hasBeam)
	{
		_beamMajorAxisRad = reader.BeamMajorAxisRad();
		_beamMinorAxisRad = reader.BeamMinorAxisRad();
		_beamPositionAngle = reader.BeamPositionAngle();
	}
	_phaseCentreDL = reader.PhaseCentreDL();
	_phaseCentreDM = reader.PhaseCentreDM();
	_origin = reader.Origin();
	_originComment = reader.OriginComment();
	_history = reader.History();
}

void FitsWriter::julianDateToYMD(double jd, int &year, int &month, int &day) const
{
  int z = jd+0.5;
  int w = (z-1867216.25)/36524.25;
  int x = w/4;
  int a = z+1+w-x;
  int b = a+1524;
  int c = (b-122.1)/365.25;
  int d = 365.25*c;
  int e = (b-d)/30.6001;
  int f = 30.6001*e;
  day = b-d-f;
  while (e-1 > 12) e-=12;
  month = e-1;
  year = c-4715-((e-1)>2?1:0);
}

void FitsWriter::MJDToHMS(double mjd, int& hour, int& minutes, int& seconds, int& deciSec)
{
	// It might seem one can calculate each of these immediately
	// without adjusting 'mjd', but this way circumvents some
	// catastrophic rounding problems, where "0:59.9" might end up
	// as "1:59.9".
	deciSec = int(fmod(mjd*36000.0 * 24.0, 10.0));
	mjd -= double(deciSec)/(36000.0 * 24.0);
	
	seconds = int(fmod(round(mjd*3600.0 * 24.0), 60.0));
	mjd -= double(seconds)/(3600.0 * 24.0);
	
	minutes = int(fmod(round(mjd*60.0 * 24.0), 60.0));
	mjd -= double(minutes)/(60.0 * 24.0);
	
	hour = int(fmod(round(mjd * 24.0), 24.0));
}

void FitsWriter::CopyDoubleKeywordIfExists(FitsReader& reader, const char* keywordName)
{
	double v;
	if(reader.ReadDoubleKeyIfExists(keywordName, v))
		SetExtraKeyword(keywordName, v);
}

void FitsWriter::CopyStringKeywordIfExists(FitsReader& reader, const char* keywordName)
{
	std::string v;
	if(reader.ReadStringKeyIfExists(keywordName, v))
		SetExtraKeyword(keywordName, v);
}
