#include <iostream>
#include <fstream>
#include "schoenberg.h"
#include <stdio.h>
#include <unistd.h>
#include <linux/input.h>

using namespace std;

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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << argv[1] << std::endl;
        cerr << "exactly one argument (not " << argc - 1 << "), which is the path to the config file, has to provided";
        return 1;
    }
    struct input_event event;
    setbuf(stdin, NULL), setbuf(stdout, NULL);
    auto config = schoenberg::read_config(argv[1]);
    auto state = schoenberg::build_state(config);

    NulOStream log_file;
    NulOStream log_written;
    NulOStream logs;

    /*
    std::ofstream log_file("/tmp/key_logs.txt", std::ios::out | std::ios::app | std::ios::ate);
    std::ofstream log_written("/tmp/key_written.txt", std::ios::out | std::ios::app | std::ios::ate);
    std::ofstream logs("/tmp/key_logs.txt", std::ios::out | std::ios::app | std::ios::ate);
     */

    while (fread(&event, sizeof(event), 1, stdin) == 1) {
        if (event.type == EV_MSC && event.code == MSC_SCAN) {
            continue;
        }
        if (event.type != EV_KEY) {
            fwrite(&event, sizeof(event), 1, stdout);
            continue;
        }

        auto res = schoenberg::process_event(state, event, logs);
        for (auto e:res) {
            log_written << "code: " << schoenberg::serialize_key(e.code) << " value: " << e.value << std::endl;
            fwrite(&e, sizeof(e), 1, stdout);
        }

        log_file << "code: " << schoenberg::serialize_key(event.code) << " value: " << event.value << std::endl;
    }

}
