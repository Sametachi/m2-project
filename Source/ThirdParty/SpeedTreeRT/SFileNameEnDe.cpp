
#include <string>
#include <assert.h>

#include "SFileNameEnDe.h"

static const char *szEncNumberTable = "1697524380";
static const char *szDecNumberTable = "9057641382";

static const char *szEncAlphaTable = "vczxmnblkjhdfgsaqwpoeiruyt";
static const char *szDecAlphaTable = "pgblumnkvjiheftsqwozxardyc";

SFileNameEnDe::SFileNameEnDe()
{
	
}

SFileNameEnDe::~SFileNameEnDe()
{

}

char enc( unsigned char c, unsigned char key )
{
	if( c >= '0' && c <= '9' ) 
	{
		c -= '0';
		c += (key%10);
		c = c % 10 + '0';
		return szEncNumberTable[ c - '0' ];
	}

	if( c >= 'a' && c <= 'z' ) 
	{
		c -= 'a';
		c += (key%26);
		c = c % 26 + 'a';
		return szEncAlphaTable[ c - 'a' ];
	}

	if( c >= 'A' && c <= 'Z' ) 
	{
		c -= 'A';
		c += (key%26);
		c = c % 26 + 'A';
		return szEncAlphaTable[ c - 'A' - ( 'A' - 'a' ) ] + ( 'A' - 'a' );
	}

	return c;
}

char dec( unsigned char c, unsigned char key )
{
	if( c >= '0' && c <= '9' ) 
	{
		c = szDecNumberTable[ c - '0' ];
		c -= '0';
		c += (10 - key%10);
		c = c % 10;
		return c + '0';
	}

	if( c >= 'a' && c <= 'z' ) 
	{
		c = szDecAlphaTable[ c - 'a' ];
		c -= 'a';
		c += (26 - key%26);
		c = c % 26;
		return c + 'a';
	}

	if( c >= 'A' && c <= 'Z' ) 
	{
		c = szDecAlphaTable[ c - 'A' - ( 'A' - 'a' ) ] + ( 'A' - 'a' );
		c -= 'A';
		c += (26 - key%26);
		c = c % 26;
		return c + 'A';
	}

	return c;
}

bool SFileNameEnDe::IsEncoded( const char * pInName )
{
	if( _strnicmp( pInName, "rc_", 3 ) ) return false;

	if( strlen( pInName ) < 7 ) return false;

	if( _stricmp( &pInName[ strlen( pInName ) - 4 ], ".rcf" ) ) return false;

	return true;
}

void SFileNameEnDe::Encode( char * pInName, char * pOutName )
{
	unsigned char key = 'a';

	strcpy( pOutName, "rc_" );
	pOutName += strlen(pOutName);

	/*
	sprintf( pOutName, "%08x", rand()*rand() );
	pOutName[0] = enc( pOutName[0], rand() );
	pOutName[1] = enc( pOutName[1], rand() );
	pOutName[2] = enc( pOutName[2], rand() );
	pOutName[3] = enc( pOutName[3], rand() );
	pOutName[4] = enc( pOutName[4], rand() );
	pOutName[5] = enc( pOutName[5], rand() );
	pOutName[6] = enc( pOutName[6], rand() );
	pOutName[7] = enc( pOutName[7], rand() );
	pOutName += 8;

	key = rand() % 26;
	*pOutName = key + 'a';
	++pOutName;
	*/

	while( *pInName )
	{
		char c = *pInName;
		if( c >= 'A' && c <= 'Z' )  c -= ('A' - 'a');

		sprintf( pOutName, "%02x", (unsigned char)c );
		pOutName[0] = enc( pOutName[0], key );
		pOutName[1] = enc( pOutName[1], key );
		pOutName += 2;
		++pInName;
	}
	*pOutName = '\0';

	strcpy( pOutName, ".rcf" );
}

void SFileNameEnDe::Encode( char * pInName, std::string & pOutName )
{
	char buf[1024];
	Encode( pInName, buf );
	pOutName = buf;	
}

void SFileNameEnDe::Decode( char * pInName, char * pOutName )
{
	char buf[2048];
	sprintf( buf, pInName );
	char *p = buf;
	int c;
	unsigned char key = 'a';

	p += 3;	// "rc_"

	/*
	p += 8;	// dummy

	char key = *p;
	key = key - 'a';
	++p;
	*/

	

	char x = enc( 'x', 5 );
	x = dec( x, 5 );

	while( *p != '.' )
	{
		p[0] = dec( p[0], key );
		p[1] = dec( p[1], key );
		sscanf( p, "%02x", &c );
		*pOutName = (unsigned char)c;
		pOutName++;
		p+=2;
	}

	*pOutName = '\0';
}

void SFileNameEnDe::Decode( char * pInName, std::string & pOutName )
{
	char buf[1024];
	Decode( pInName, buf );
	pOutName = buf;
}
