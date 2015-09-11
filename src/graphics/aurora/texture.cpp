/* xoreos - A reimplementation of BioWare's Aurora engine
 *
 * xoreos is the legal property of its developers, whose names
 * can be found in the AUTHORS file distributed with this source
 * distribution.
 *
 * xoreos is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * xoreos is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xoreos. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  A texture as used in the Aurora engines.
 */

#include <cassert>

#include "src/common/types.h"
#include "src/common/util.h"
#include "src/common/error.h"
#include "src/common/readstream.h"

#include "src/graphics/aurora/texture.h"
#include "src/graphics/aurora/pltfile.h"

#include "src/graphics/types.h"
#include "src/graphics/graphics.h"
#include "src/graphics/images/txi.h"
#include "src/graphics/images/decoder.h"
#include "src/graphics/images/tga.h"
#include "src/graphics/images/dds.h"
#include "src/graphics/images/tpc.h"
#include "src/graphics/images/txb.h"
#include "src/graphics/images/sbm.h"
#include "src/graphics/images/xoreositex.h"

#include "src/events/requests.h"

#include "src/aurora/resman.h"

using Events::RequestID;

namespace Graphics {

namespace Aurora {

Texture::Texture() : _type(::Aurora::kFileTypeNone), _image(0), _txi(0), _width(0), _height(0) {
}

Texture::Texture(const Common::UString &name, ImageDecoder *image, ::Aurora::FileType type, TXI *txi) :
	_name(name), _type(type), _image(0), _txi(0), _width(0), _height(0) {

	set(name, image, type, txi);
	addToQueues();
}

Texture::~Texture() {
	removeFromQueues();

	if (_textureID != 0)
		GfxMan.abandon(&_textureID, 1);

	delete _txi;
	delete _image;
}

uint32 Texture::getWidth() const {
	return _width;
}

uint32 Texture::getHeight() const {
	return _height;
}

bool Texture::hasAlpha() const {
	if (!_image)
		return false;

	return _image->hasAlpha();
}

bool Texture::isDynamic() const {
	return false;
}

static const TXI kEmptyTXI;
const TXI &Texture::getTXI() const {
	if (_txi)
		return *_txi;

	if (_image)
		return _image->getTXI();

	return kEmptyTXI;
}

const ImageDecoder &Texture::getImage() const {
	assert(_image);

	return *_image;
}

bool Texture::reload() {
	if (_name.empty())
		return false;

	::Aurora::FileType type = ::Aurora::kFileTypeNone;
	ImageDecoder *image = 0;
	TXI *txi = 0;

	try {

		txi   = loadTXI  (_name);
		image = loadImage(_name, type, txi);

	} catch (Common::Exception &e) {
		delete txi;

		e.add("Failed to reload texture \"%s\" (%d)", _name.c_str(), type);
		throw;
	}

	removeFromQueues();
	set(_name, image, type, txi);
	addToQueues();

	return true;
}

bool Texture::dumpTGA(const Common::UString &fileName) const {
	if (!_image)
		return false;

	return _image->dumpTGA(fileName);
}

void Texture::doDestroy() {
	if (_textureID == 0)
		return;

	glDeleteTextures(1, &_textureID);

	_textureID = 0;
}

void Texture::doRebuild() {
	if (!_image)
		// No image
		return;

	// Generate the texture ID
	if (_textureID == 0)
		glGenTextures(1, &_textureID);

	if (_image->isCubeMap()) {
		createCubeMapTexture();
		return;
	}

	create2DTexture();
}

void Texture::create2DTexture() {
	// Bind the texture
	glBindTexture(GL_TEXTURE_2D, _textureID);

	// Texture wrapping
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Filter?
	const TXI::Features &features = getTXI().getFeatures();
	if (features.filter) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

	if (_image->getMipMapCount() == 1) {
		// Texture doesn't specify any mip maps, generate our own

		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 9);
	} else {
		// Texture does specify mip maps, use these

		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, _image->getMipMapCount() - 1);
	}

	// Texture image data
	if (_image->isCompressed()) {
		// Compressed texture data

		for (size_t i = 0; i < _image->getMipMapCount(); i++) {
			const ImageDecoder::MipMap &mipMap = _image->getMipMap(i);

			glCompressedTexImage2D(GL_TEXTURE_2D, i, _image->getFormatRaw(),
			                       mipMap.width, mipMap.height, 0,
			                       mipMap.size, mipMap.data);
		}

	} else {
		// Uncompressed texture data

		for (size_t i = 0; i < _image->getMipMapCount(); i++) {
			const ImageDecoder::MipMap &mipMap = _image->getMipMap(i);

			glTexImage2D(GL_TEXTURE_2D, i, _image->getFormatRaw(),
			             mipMap.width, mipMap.height, 0, _image->getFormat(),
			             _image->getDataType(), mipMap.data);
		}

	}
}

void Texture::createCubeMapTexture() {
	glBindTexture(GL_TEXTURE_CUBE_MAP, _textureID);

	// Texture wrapping
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Filter?
	const TXI::Features &features = getTXI().getFeatures();
	if (features.filter) {
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	} else {
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

	assert(_image->getLayerCount() == 6);

	static const GLenum faceTarget[6] = {
		GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	};

	for (size_t i = 0; i < _image->getLayerCount(); i++) {
		if (_image->getMipMapCount() == 1) {
			// Texture doesn't specify any mip maps, generate our own

			glTexParameteri(faceTarget[i], GL_GENERATE_MIPMAP, GL_TRUE);
			glTexParameteri(faceTarget[i], GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(faceTarget[i], GL_TEXTURE_MAX_LEVEL, 9);
		} else {
			// Texture does specify mip maps, use these

			glTexParameteri(faceTarget[i], GL_GENERATE_MIPMAP, GL_FALSE);
			glTexParameteri(faceTarget[i], GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(faceTarget[i], GL_TEXTURE_MAX_LEVEL, _image->getMipMapCount() - 1);
		}

		// Texture image data
		if (_image->isCompressed()) {
			// Compressed texture data

			for (size_t j = 0; j < _image->getMipMapCount(); j++) {
				const ImageDecoder::MipMap &mipMap = _image->getMipMap(j, i);

				glCompressedTexImage2D(faceTarget[i], j, _image->getFormatRaw(),
				                       mipMap.width, mipMap.height, 0,
				                       mipMap.size, mipMap.data);
			}

		} else {
			// Uncompressed texture data

			for (size_t j = 0; j < _image->getMipMapCount(); j++) {
				const ImageDecoder::MipMap &mipMap = _image->getMipMap(j, i);

				glTexImage2D(faceTarget[i], j, _image->getFormatRaw(),
				             mipMap.width, mipMap.height, 0, _image->getFormat(),
				             _image->getDataType(), mipMap.data);
			}

		}
	}
}

Texture *Texture::createPLT(const Common::UString &name, Common::SeekableReadStream *imageStream) {
	Texture *texture = 0;
	try {
		texture = new PLTFile(name, *imageStream);
	} catch (...) {
		delete imageStream;
		throw;
	}

	delete imageStream;
	return texture;
}

Texture *Texture::create(const Common::UString &name) {
	::Aurora::FileType type = ::Aurora::kFileTypeNone;
	ImageDecoder *image = 0;
	TXI *txi = 0;

	try {
		Common::SeekableReadStream *imageStream = ResMan.getResource(::Aurora::kResourceImage, name, &type);
		if (!imageStream)
			throw Common::Exception("No such image resource \"%s\"", name.c_str());

		// PLT needs extra handling, since they're their own Texture class
		if (type == ::Aurora::kFileTypePLT)
			return createPLT(name, imageStream);

		txi   = loadTXI(name);
		image = loadImage(imageStream, type, txi);

	} catch (Common::Exception &e) {
		e.add("Failed to create texture \"%s\" (%d)", name.c_str(), type);
		throw;
	}

	return new Texture(name, image, type, txi);
}

Texture *Texture::create(ImageDecoder *image, ::Aurora::FileType type, TXI *txi) {
	if (!image)
		throw Common::Exception("Can't create a texture from an empty image");

	if (image->getMipMapCount() < 1)
		throw Common::Exception("Texture has no images");

	return new Texture("", image, type, txi);
}

void Texture::set(const Common::UString &name, ImageDecoder *image, ::Aurora::FileType type, TXI *txi) {
	delete _image;
	delete _txi;

	_name   = name;
	_image  = image;
	_type   = type;
	_txi    = txi;

	_width  = _image->getMipMap(0).width;
	_height = _image->getMipMap(0).height;
}

ImageDecoder *Texture::loadImage(const Common::UString &name) {
	::Aurora::FileType type;

	return loadImage(name, type);
}

void Texture::addToQueues() {
	addToQueue(kQueueTexture);
	addToQueue(kQueueNewTexture);
}

void Texture::removeFromQueues() {
	removeFromQueue(kQueueNewTexture);
	removeFromQueue(kQueueTexture);
}

void Texture::refresh() {
	removeFromQueues();
	addToQueues();
}

ImageDecoder *Texture::loadImage(const Common::UString &name, ::Aurora::FileType &type) {
	return loadImage(name, type, 0);
}

ImageDecoder *Texture::loadImage(const Common::UString &name, ::Aurora::FileType &type, TXI *txi) {
	Common::SeekableReadStream *imageStream = ResMan.getResource(::Aurora::kResourceImage, name, &type);
	if (!imageStream)
		throw Common::Exception("No such image resource \"%s\"", name.c_str());

	return loadImage(imageStream, type, txi);
}

ImageDecoder *Texture::loadImage(Common::SeekableReadStream *imageStream, ::Aurora::FileType type,
                                 TXI *txi) {

	// Check for a cube map, but only those that don't use a file for each side
	const bool isCubeMap = txi && txi->getFeatures().cube && (txi->getFeatures().fileRange == 0);

	ImageDecoder *image = 0;
	try {
		// Loading the different image formats
		if      (type == ::Aurora::kFileTypeTGA)
			image = new TGA(*imageStream, isCubeMap);
		else if (type == ::Aurora::kFileTypeDDS)
			image = new DDS(*imageStream);
		else if (type == ::Aurora::kFileTypeTPC)
			image = new TPC(*imageStream);
		else if (type == ::Aurora::kFileTypeTXB)
			image = new TXB(*imageStream);
		else if (type == ::Aurora::kFileTypeSBM)
			image = new SBM(*imageStream);
		else if (type == ::Aurora::kFileTypeXEOSITEX)
			image = new XEOSITEX(*imageStream);
		else
			throw Common::Exception("Unsupported image resource type %d", (int) type);

		if (image->getMipMapCount() < 1)
			throw Common::Exception("Texture has no images");

		// Decompress
		if (GfxMan.needManualDeS3TC())
			image->decompress();

	} catch (...) {
		delete image;
		delete imageStream;

		throw;
	}

	delete imageStream;
	return image;
}

TXI *Texture::loadTXI(const Common::UString &name) {
	Common::SeekableReadStream *txiStream = ResMan.getResource(name, ::Aurora::kFileTypeTXI);
	if (!txiStream)
		return 0;

	TXI *txi = 0;
	try {
		txi = new TXI(*txiStream);
	} catch (Common::Exception &e) {
		e.add("Failed loading TXI \"%s\"", name.c_str());
		Common::printException(e, "WARNING: ");
	}

	delete txiStream;
	return txi;
}

} // End of namespace Aurora

} // End of namespace Graphics
