/* eos - A reimplementation of BioWare's Aurora engine
 * Copyright (c) 2010 Sven Hesse (DrMcCoy), Matthew Hoops (clone2727)
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 *
 * This file is part of eos and is distributed under the terms of
 * the GNU General Public Licence. See COPYING for more informations.
 */

#ifndef AURORA_GFFFILE_H
#define AURORA_GFFFILE_H

#include <vector>

#include "common/types.h"

#include "aurora/types.h"

namespace Common {
	class SeekableReadStream;
}

namespace Aurora {

class LocString;

/** A GFF header. */
struct GFFHeader {
	uint32 id;
	uint32 version;
	uint32 structOffset;
	uint32 structCount;
	uint32 fieldOffset;
	uint32 fieldCount;
	uint32 labelOffset;
	uint32 labelCount;
	uint32 fieldDataOffset;
	uint32 fieldDataCount;
	uint32 fieldIndicesOffset;
	uint32 fieldIndicesCount;
	uint32 listIndicesOffset;
	uint32 listIndicesCount;

	GFFHeader();

	/** Clear the header. */
	void clear();

	/** Read the header out of a stream. */
	bool read(Common::SeekableReadStream &gff);
};

/** A data field found in a GFF. */
class GFFField {
public:
	/** The general type of the field's value. */
	enum Type {
		kTypeNone        = 0,
		kTypeChar        = 1,
		kTypeUint        = 2,
		kTypeSint        = 3,
		kTypeDouble      = 4,
		kTypeString      = 5,
		kTypeLocString   = 6,
		kTypeData        = 7,
		kTypeStruct      = 8,
		kTypeList        = 9,
		kTypeOrientation = 10,
		kTypeVector      = 11
	};

	GFFField();
	~GFFField();

	/** Clear the field. */
	void clear();

	/** Return the field's type. */
	Type getType() const;

	/** Get the field's label. */
	const std::string &getLabel() const;

	char   getChar() const;
	uint64 getUint() const;
	int64  getSint() const;

	double getDouble() const;

	const std::string &getString()    const;
	const LocString   &getLocString() const;

	uint32 getDataSize() const;
	const byte *getData() const;

	const float *getVector() const;

	const uint32 getIndex() const;

	/** Read the field out of a stream. */
	bool read(Common::SeekableReadStream &gff, const GFFHeader &header);

private:
	/** The actual type of the field, as found in the GFF. */
	enum GFFType {
		kGFFTypeNone        = - 1, ///< Invalid type.
		kGFFTypeByte        =   0, ///< A single byte.
		kGFFTypeChar        =   1, ///< A single character.
		kGFFTypeUint16      =   2, ///< Unsigned 16bit integer.
		kGFFTypeSint16      =   3, ///< Signed 16bit integer.
		kGFFTypeUint32      =   4, ///< Unsigned 32bit integer.
		kGFFTypeSint32      =   5, ///< Signed 32bit integer.
		kGFFTypeUint64      =   6, ///< Unsigned 64bit integer.
		kGFFTypeSint64      =   7, ///< Signed 64bit integer.
		kGFFTypeFloat       =   8, ///< IEEE float.
		kGFFTypeDouble      =   9, ///< IEEE double.
		kGFFTypeExoString   =  10, ///< String.
		kGFFTypeResRef      =  11, ///< String, max. 16 characters.
		kGFFTypeLocString   =  12, ///< Localized string.
		kGFFTypeVoid        =  13, ///< Random data of variable length.
		kGFFTypeStruct      =  14, ///< Struct containing a number of fields.
		kGFFTypeList        =  15, ///< List containing a number of structs.
		kGFFTypeOrientation =  16, ///< An object orientation.
		kGFFTypeVector      =  17, ///< A vector of 3 floats.
		kGFFTypeStrRef      =  18  // TODO: New in Jade Empire
	};

	GFFType _gffType; ///< The field's actual type.
	   Type _type;    ///< The field's general type.

	std::string _label; ///< The field's label.

	uint32 _dataSize; ///< The size of the field's data, if applicable.

	/** The field's value. */
	union {
		uint64 typeInt;
		double typeDouble;

		std::string *typeString;
		LocString   *typeLocString;

		byte *typeData;

		float typeVector[3];

		uint32 typeIndex;
	} _value;

	// Reading helpers

	bool convertData(Common::SeekableReadStream &gff, const GFFHeader &header, uint32 data);

	inline bool readUint64   (Common::SeekableReadStream &gff,
			const GFFHeader &header, uint32 data);
	inline bool readSint64   (Common::SeekableReadStream &gff,
			const GFFHeader &header, uint32 data);
	inline bool readDouble   (Common::SeekableReadStream &gff,
			const GFFHeader &header, uint32 data);
	inline bool readExoString(Common::SeekableReadStream &gff,
			const GFFHeader &header, uint32 data);
	inline bool readResRef   (Common::SeekableReadStream &gff,
			const GFFHeader &header, uint32 data);
	inline bool readLocString(Common::SeekableReadStream &gff,
			const GFFHeader &header, uint32 data);
	inline bool readVoid     (Common::SeekableReadStream &gff,
			const GFFHeader &header, uint32 data);
	inline bool readVector   (Common::SeekableReadStream &gff,
			const GFFHeader &header, uint32 data);

	inline bool seekGFFData(Common::SeekableReadStream &gff,
			const GFFHeader &header, uint32 data, uint32 &curPos);

	/** Convert an actual GFF field type to a general type. */
	static inline Type toType(GFFType type);
};

/** A GFF, BioWare's General File Format. */
class GFFFile {
public:
	GFFFile();
	~GFFFile();

	/** Clear all information. */
	void clear();

	/** Load the GFF out of a stream. */
	bool load(Common::SeekableReadStream &gff);

private:
	typedef std::vector<GFFField>  GFFStruct;
	typedef std::vector<GFFStruct> GFFStructArray;

	typedef std::vector<uint32> GFFListArray;

	GFFHeader _header;

	GFFStructArray _structArray;
	GFFListArray   _listArray;

	// Reading helpers
	bool readField(Common::SeekableReadStream &gff, GFFField &field, uint32 fieldIndex);
	bool readFields(Common::SeekableReadStream &gff, GFFStruct &strct, uint32 fieldIndicesIndex);
};

} // End of namespace Aurora

#endif // AURORA_GFFFILE_H
