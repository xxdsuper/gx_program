#pragma once

#include "rapidjson/document.h"
#include "rapidjson/reader.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/encodings.h"

#include <string.h>


using namespace rapidjson;
using namespace std;

typedef struct _JSON_OBJECT_CONTENT {
	string		strValue;
	bool		bValue;
}JSON_OJBCONTENT, *LPSJON_OBJECT_CONTENT;

class CJsonHelper {
public:
	CJsonHelper() {}
	~CJsonHelper() {}
public:
};
