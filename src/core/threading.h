// Copyright (c) 2014 Dropbox, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef PYSTON_CORE_THREADING_H
#define PYSTON_CORE_THREADING_H

namespace pyston {
namespace threading {

#define THREADING_USE_GIL 1
#define THREADING_USE_GRWL 0
#define THREADING_SAFE_DATASTRUCTURES THREADING_USE_GRWL

void acquireGLRead();
void releaseGLRead();
void acquireGLWrite();
void releaseGLWrite();
// Note: promoteGL is free to drop the lock and then reacquire
void promoteGL();
void demoteGL();



#if THREADING_USE_GIL
inline void acquireGLRead() {
    acquireGLWrite();
}
inline void releaseGLRead() {
    releaseGLWrite();
}
inline void promoteGL() {
}
inline void demoteGL() {
}
#endif

#define MAKE_REGION(name, start, end) \
class name {\
public:\
    name() { start(); }\
    ~name() { end(); }\
};

MAKE_REGION(GLReadRegion, acquireGLRead, releaseGLRead);
MAKE_REGION(GLPromoteRegion, promoteGL, demoteGL);
MAKE_REGION(GLReadReleaseRegion, releaseGLRead, acquireGLRead);
MAKE_REGION(GLWriteReleaseRegion, releaseGLWrite, acquireGLWrite);
#undef MAKE_REGION

} // namespace threading
} // namespace pyston

#endif