/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2018 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/
#ifndef INTRECT_H
#define INTRECT_H

class IntRect {
 public:
  IntRect()
    : m_x(0), m_y(0), m_width(0), m_height(0) { }
  IntRect(int x, int y, int width, int height)
    : m_x(x), m_y(y), m_width(width), m_height(height) { }

  int x() const { return m_x; }
  int y() const { return m_y; }
  int width() const { return m_width; }
  int height() const { return m_height; }

 private:
  int m_x;
  int m_y;
  int m_width;
  int m_height;
};

inline bool operator==(const IntRect& a, const IntRect& b) {
  return
    a.x() == b.x() && a.y() == b.y() &&
    a.width() == b.width() && a.height() == b.height();
}

inline bool operator!=(const IntRect& a, const IntRect& b) {
  return
    a.x() != b.x() || a.y() != b.y() ||
    a.width() != b.width() || a.height() != b.height();
}

#endif  // _INTRECT_H_
