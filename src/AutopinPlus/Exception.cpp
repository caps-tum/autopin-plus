/*
 * This file is part of Autopin+.
 * Copyright (C) 2015 Technische Universität München - LRR
 *
 * This file is licensed under the GNU General Public License Version 3
 */

#include <AutopinPlus/Exception.h>

#include <qbytearray.h> // for QByteArray
#include <qstring.h>	// for QString

namespace AutopinPlus {
Exception::Exception(QString message) noexcept { this->message = message.toUtf8(); }

Exception::~Exception() noexcept {}

const char *Exception::what() const noexcept { return this->message.data(); }
} // namespace AutopinPlus
