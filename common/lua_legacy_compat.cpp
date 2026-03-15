#if defined(_WIN32) && defined(_MSC_VER)

#include <stdio.h>

extern "C" {

__declspec(dllimport) FILE* __cdecl __acrt_iob_func(unsigned index);
int __cdecl _get_fmode(int* mode);

FILE* __cdecl __iob_func(void) {
    return __acrt_iob_func(0);
}

int _fmode = 0;

}

namespace {
struct legacy_fmode_sync {
    legacy_fmode_sync() {
        int mode = 0;
        if (_get_fmode(&mode) == 0) {
            _fmode = mode;
        }
    }
};

legacy_fmode_sync g_legacy_fmode_sync;
} // namespace

#endif
