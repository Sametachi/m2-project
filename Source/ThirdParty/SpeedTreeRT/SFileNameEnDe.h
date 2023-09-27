
#pragma once

//화일이름을 임의의 이름으로 변경, 반대도 지원
//임의의 화일 이름이 겹치면 안된다. 가능성은???

class SFileNameEnDe
{
public:
	SFileNameEnDe();
	virtual ~SFileNameEnDe();

	static void Encode( char * pInName, char * pOutName );
	static void Encode( char * pInName, std::string & pOutName );
	static void Decode( char * pInName, char * pOutName );
	static void Decode( char * pInName, std::string & pOutName );
	static bool IsEncoded( const char * pInName );
};