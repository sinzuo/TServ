#include "stdafx.h"
#include "json.h"
#include <string.h>

unsigned int ClearJsonBlank(const char *src, char *dst)
{
	char *strptr = (char *)src;
	char *dstptr = dst;

	while(*strptr)
	{
		if((*strptr) == '\"')
		{
			char *endstr = strchr(strptr + 1, '\"');
			if(endstr == NULL)
				return 0;
			memcpy(dstptr, strptr, endstr - strptr + 1);
			strptr = endstr + 1;
			dstptr += strlen(dstptr);
		}
		else if((*strptr) == ' ' || (*strptr) == '\r' || (*strptr) == '\n' || (*strptr) == '\t')
			strptr ++;
		else
			*(dstptr ++) = *(strptr ++);
	}
	return strlen(dst);
}

unsigned int GetSubString(char *ststr, char *endstr, std::string &outstr)
{
	char outbuff[4096] = {0};
	memcpy(outbuff, ststr, endstr - ststr + 1);
	outstr.append(outbuff);
	return endstr - ststr + 1;
}

JsonItem::JsonItem()
{
	SubItemCount = 0;
	memset(Name, 0, 32);
	StringValue = "";
	SubItemType = 0;
}

JsonItem::~JsonItem()
{
	SubItemCount = 0;
	memset(Name, 0, 32);
	StringValue = "";
	SubItemType = 0;
	SubItems.clear();
}

int JsonItem::GetAttrByName(const char *attrname, char *value)
{
	if(attrname == NULL || value == NULL)
		return 0;

	if(!attrname[0])
		return 0;

	int itemcount = SubItems.size();
	for(int i = 0; i < itemcount; i ++)
	{
		if(SubItems[i].Name[0] && strcmp(attrname, SubItems[i].Name) == NULL)
		{
			memset(value, 0, SubItems[i].StringValue.length() + 1);
			strcpy(value, SubItems[i].StringValue.c_str());
			return 1;
		}
	}

	return 0;
}

int JsonItem::GetAttrByName(const char *attrname, char *value, unsigned int length)
{
	if(attrname == NULL || value == NULL)
		return 0;

	if(!attrname[0])
		return 0;

	int itemcount = SubItems.size();
	for(int i = 0; i < itemcount; i ++)
	{
		if(SubItems[i].Name[0] && strcmp(attrname, SubItems[i].Name) == NULL)
		{
			memset(value, 0, SubItems[i].StringValue.length() + 1);
			strncpy(value, SubItems[i].StringValue.c_str(), length);
			return 1;
		}
	}

	return 0;
}

int JsonItem::GetAttrByName(const char *attrname, unsigned int *value)
{
	char buff[128] = {0};

	if(!GetAttrByName(attrname, buff))
		return 0;

	*value = atoi(buff);
	return 1;
}

int JsonItem::GetAttrByName(const char *attrname, unsigned char *value)
{
	char buff[128] = {0};

	if(!GetAttrByName(attrname, buff))
		return 0;

	*value = atoi(buff);
	return 1;
}

int JsonItem::GetAttrByName(const char *attrname, unsigned short *value)
{
	char buff[128] = {0};

	if(!GetAttrByName(attrname, buff))
		return 0;

	*value = atoi(buff);
	return 1;
}

int JsonItem::GetSubItemByName(const char *attrname, JsonItem **value)
{
	if(attrname == NULL || value == NULL)
		return 0;

	if(!attrname[0])
		return 0;

	int itemcount = SubItems.size();
	for(int i = 0; i < itemcount; i ++)
	{
		if(SubItems[i].Name[0] && strcmp(attrname, SubItems[i].Name) == NULL)
		{
			*value = &SubItems[i];
			return 1;
		}
	}

	return 0;
}

int JsonItem::AddAttr(const char *attrname, char *value)
{
	char vl[1024] = {0};

	if(attrname == NULL || value == NULL)
		return 0;

	if(!attrname[0])
		return 0;

	if(GetAttrByName(attrname, vl))
		return 0;

	JsonItem item;
	strcpy(item.Name, attrname);
	item.SubItemType = JSON_ITEM_TYPE_STRING;
	item.StringValue = value;

	SubItemCount ++;
	SubItems.push_back(item);

	return 1;
}

char *JsonItem::LoadFromString(const char *jsonstr)
{
	char *char_ptr = (char *)jsonstr;

	if((*char_ptr) == '[')
	{
		SubItemType = JSON_ITEM_TYPE_ARRAY;

		char_ptr ++;
		if((*char_ptr) == ']')
		{
			GetSubString((char *)jsonstr, char_ptr, JsonString);
			return char_ptr + 1;
		}
		{
			JsonItem item;
			if((char_ptr = item.LoadFromString(char_ptr)) == 0)
				return 0;
			SubItemCount ++;
			SubItems.push_back(item);
		}

		while((*char_ptr) == ',')
		{
			char_ptr ++;
			JsonItem item;
			if((char_ptr = item.LoadFromString(char_ptr)) == 0)
				return 0;
			SubItemCount ++;
			SubItems.push_back(item);
		}

		if((*char_ptr) != ']')
			return 0;
	}
	else if((*char_ptr) == '{')
	{
		SubItemType = JSON_ITEM_TYPE_OBJECT;

		char_ptr ++;
		if((*char_ptr) == '}')
		{
			GetSubString((char *)jsonstr, char_ptr, JsonString);
			return char_ptr + 1;
		}
		while(*char_ptr)
		{
			if((*char_ptr) == '\"')
			{
				JsonItem item;
				{
					char_ptr ++;
					char *endptr = strchr(char_ptr, '\"');
					if(endptr == NULL)
						return 0;
					memcpy(item.Name, char_ptr, ((endptr - char_ptr) < 32)?(endptr - char_ptr):31);
					char_ptr = endptr + 1;
				}
				if((*char_ptr) != '=' && (*char_ptr) != ':')
					return 0;

				char_ptr ++;
				// 此为等号右边的内容
				if((*char_ptr) == '\"')
				{
					char strbuff[1024] = {0};
					char_ptr ++;
					char *endptr = strchr(char_ptr, '\"');
					if(endptr == NULL)
						return 0;
					memcpy(strbuff, char_ptr, ((endptr - char_ptr) < 1024)?(endptr - char_ptr):1023);
					item.StringValue.append(strbuff);
					char_ptr = endptr + 1;

					item.SubItemType = JSON_ITEM_TYPE_STRING;

					SubItemCount ++;
					SubItems.push_back(item);
				}
				else if((*char_ptr) == '.' || ((*char_ptr) >= '0' && (*char_ptr) <= '9'))
				{
					char strbuff[1024] = {0};
					char *endptr = strbuff;
					while((*char_ptr) == '.' || ((*char_ptr) >= '0' && (*char_ptr) <= '9'))
						*(endptr ++) = *(char_ptr ++);
						char_ptr ++;

					item.StringValue.append(strbuff);

					item.SubItemType = JSON_ITEM_TYPE_NUMBER;

					SubItemCount ++;
					SubItems.push_back(item);
				}
				else if((*char_ptr) == '{' || (*char_ptr) == '[')
				{
					if((char_ptr = item.LoadFromString(char_ptr)) == 0)
						return 0;
					SubItemCount ++;
					SubItems.push_back(item);
				}
			}
			else if((*char_ptr) == '}')
			{
				GetSubString((char *)jsonstr, char_ptr, JsonString);
				return char_ptr + 1;
			}
			else if((*char_ptr) == ',')
				char_ptr ++;
			else
				return 0;
		}
	}
	else
		return 0;

	GetSubString((char *)jsonstr, char_ptr, JsonString);
	return char_ptr + 1;
}

char *JsonItem::LoadFromStringF(const char *jsonstr)
{
	char buff[4096] = {0};
	ClearJsonBlank(jsonstr, buff);
	return LoadFromString(buff);
}
