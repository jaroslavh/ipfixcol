/**
 * \file Values.h
 * \author Petr Velan <petr.velan@cesnet.cz>
 * \brief Header of struct for managing values of different types
 *
 * Copyright (C) 2011 CESNET, z.s.p.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is, and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */

#ifndef VALUES_H_
#define VALUES_H_

#include "typedefs.h"

namespace fbitdump {

#define MAX_PARTS 2

/**
 * \brief structure for passing values of unknown type
 */
class Values
{
public:
	ibis::TYPE_T type;
	union
	{
		char int8;
		unsigned char uint8;
		int16_t int16;
		uint16_t uint16;
		int32_t int32;
		uint32_t uint32;
		int64_t int64;
		uint64_t uint64;
		float flt;
		double dbl;
	} value[MAX_PARTS];
	std::string string;

	/**
	 * \brief Convert value to int64_t type
	 *
	 * @param part which part to convert
	 * @return int64_t converted value
	 */
	int64_t toLong(int part=0) const;

	/**
	 * \brief Convert value to double type
	 *
	 * @param part which part to convert
	 * @return converted value of type double
	 */
	double toDouble(int part=0) const;

	/**
	 * \brief Return string representation of value
	 *
	 * @param plainNumbers Don't use M,G format for long numbers
	 * @return String representation of value
	 */
	std::string toString(bool plainNumbers) const;

	/* comparison operators overload */
	friend bool operator==(const Values &lhs, const Values &rhs);
	friend bool operator< (const Values &lhs, const Values &rhs);
	friend bool operator!=(const Values &lhs, const Values &rhs);
	friend bool operator> (const Values &lhs, const Values &rhs);
	friend bool operator>= (const Values &lhs, const Values &rhs);
	friend bool operator<= (const Values &lhs, const Values &rhs);

private:
	/**
	 * \brief Formats number 'num' to ostringstream 'ss'
	 *
 	 * Uses precision 1 if output has units and 0 otherwise
 	 * doesn't format if 'plainNumbers' is set
 	 *
 	 * When defined as macro, it can be a bit quicker
 	 *
	 * @param num number to format
	 * @param ss string strem to put result to
	 * @param plainNumbers whether to format or not
	 */
	template<class T>
	inline void formatNumber(T num, std::ostringstream &ss, bool plainNumbers) const;
};

} /* end of namespace fbitdump */

#endif /* VALUES_H_ */
