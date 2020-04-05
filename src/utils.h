#include <unistd.h>
#include <iostream>
#include <fstream>

class NulStreambuf : public std::streambuf
{
    char                dummyBuffer[ 64 ];
protected:
    virtual int         overflow( int c )
    {
        setp( dummyBuffer, dummyBuffer + sizeof( dummyBuffer ) );
        return (c == traits_type::eof()) ? '\0' : c;
    }
};

class NulOStream : private NulStreambuf, public std::ostream
{
public:
    NulOStream() : std::ostream( this ) {}
};

