// Copyright 2011 Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
// * Neither the name of Google Inc. nor the names of its contributors
//   may be used to endorse or promote products derived from this software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "engine/isolation.ipp"

extern "C" {
#include <unistd.h>
}

#include <cstring>

#include "engine/exceptions.hpp"
#include "utils/env.hpp"
#include "utils/format/macros.hpp"
#include "utils/fs/operations.hpp"
#include "utils/optional.ipp"

namespace fs = utils::fs;

using utils::optional;


namespace {


/// Number of the stop signal.
///
/// This is set by interrupt_handler() when it receives a signal that ought to
/// terminate the execution of the current test case.
static int interrupted_signo = 0;


}  // anonymous namespace


/// Atomically creates a new work directory with a unique name.
///
/// The directory is created under the system-wide configured temporary
/// directory as defined by the TMPDIR environment variable.
///
/// \return The path to the new work directory.
///
/// \throw fs::error If there is a problem creating the temporary directory.
fs::path
engine::create_work_directory(void)
{
    const optional< std::string > tmpdir = utils::getenv("TMPDIR");
    if (!tmpdir)
        return fs::mkdtemp(fs::path("/tmp/kyua.XXXXXX"));
    else
        return fs::mkdtemp(fs::path(F("%s/kyua.XXXXXX") % tmpdir.get()));
}


/// Signal handler for termination signals.
///
/// \param signo The signal received.
///
/// \post interrupted_signo is set to the received signal.
void
engine::detail::interrupt_handler(const int signo)
{
    const char* message = "[-- Signal caught; please wait for clean up --]\n";
    if (::write(STDERR_FILENO, message, std::strlen(message)) == -1) {
        // We are exiting: the message printed here is only for informational
        // purposes.  If we fail to print it (which probably means something
        // is really bad), there is not much we can do within the signal
        // handler, so just ignore this.
    }
    interrupted_signo = signo;

    POST(interrupted_signo != 0);
    POST(interrupted_signo == signo);
}


/// Syntactic sugar to validate if there is a pending signal.
///
/// \throw interrupted_error If there is a pending signal that ought to
///     terminate the execution of the program.
void
engine::check_interrupt(void)
{
    LD("Checking for pending interrupt signals");
    if (interrupted_signo != 0) {
        LI("Interrupt pending; raising error to cause cleanup");
        throw engine::interrupted_error(interrupted_signo);
    }
}
