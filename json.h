#ifndef JSON_H
#define JSON_H

#include <string>
#include <vector>

#define SKIPBLANK(ptr) while((*(ptr))==' '||(*(ptr))=='\r'||(*(ptr))=='\n'||(*(ptr))=='\t'){ptr++;}

#define JSON_ITEM_TYPE_STRING 0
#define JSON_ITEM_TYPE_NUMBER 1
#define JSON_ITEM_TYPE_OBJECT 2
#define JSON_ITEM_TYPE_ARRAY 3

class JsonItem
{
public:
	int SubItemCount;

	int SubItemType;

	char Name[32];

	std::string StringValue;
	std::string JsonString;
	std::vector <JsonItem> SubItems;

	JsonItem();
	~JsonItem();

	char *LoadFromString(const char *jsonstr);
	char *LoadFromStringF(const char *jsonstr);

	int GetAttrByName(const char *attrname, char *value);
	int GetAttrByName(const char *attrname, char *value, unsigned int length);
	int GetAttrByName(const char *attrname, unsigned int *value);
	int GetAttrByName(const char *attrname, unsigned short *value);
	int GetAttrByName(const char *attrname, unsigned char *value);

	int GetSubItemByName(const char *attrname, JsonItem **value);

	int AddAttr(const char *attrname, char *value);
};

#endif
