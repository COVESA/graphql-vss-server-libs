#include "debug.hpp"

#if GRAPHQL_VSS_SERVER_LIBS_SUPPORT_DEBUG
// NOTE: this must be before the first dbg() user, so keep this file the first to be compiled!
// otherwise you may get the lock to be cleaned up BEFORE the users, resulting in:
//    terminating with uncaught exception of type std::__1::system_error: mutex lock failed: Invalid
//    argument
std::mutex _dbg_lock;
#endif
