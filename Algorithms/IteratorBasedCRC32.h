/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */


#ifndef ITERATORBASEDCRC32_H_
#define ITERATORBASEDCRC32_H_

#include <boost/crc.hpp>  // for boost::crc_32_type
#include <iostream>

template<class ContainerT>
class IteratorbasedCRC32 {
private:
    typedef typename ContainerT::iterator ContainerT_iterator;
    unsigned crc;

    typedef boost::crc_optimal<32, 0x1EDC6F41, 0x0, 0x0, true, true> my_crc_32_type;
    typedef unsigned (IteratorbasedCRC32::*CRC32CFunctionPtr)(char *str, unsigned len, unsigned crc);

    unsigned SoftwareBasedCRC32(char *str, unsigned len, unsigned ){
        boost::crc_optimal<32, 0x1EDC6F41, 0x0, 0x0, true, true> CRC32_Processor;
        CRC32_Processor.process_bytes( str, len);
        return CRC32_Processor.checksum();
    }
    unsigned SSEBasedCRC32( char *str, unsigned len, unsigned crc){
        unsigned q=len/sizeof(unsigned),
                r=len%sizeof(unsigned),
                *p=(unsigned*)str/*, crc*/;

        //crc=0;
        while (q--) {
            __asm__ __volatile__(
                    ".byte 0xf2, 0xf, 0x38, 0xf1, 0xf1;"
                    :"=S"(crc)
                     :"0"(crc), "c"(*p)
            );
            ++p;
        }

        str=(char*)p;
        while (r--) {
            __asm__ __volatile__(
                    ".byte 0xf2, 0xf, 0x38, 0xf1, 0xf1;"
                    :"=S"(crc)
                     :"0"(crc), "c"(*str)
            );
            ++str;
        }
        return crc;
    }

    unsigned cpuid(unsigned functionInput){
        unsigned eax;
        unsigned ebx;
        unsigned ecx;
        unsigned edx;
        asm("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (functionInput));
        return ecx;
    }

    CRC32CFunctionPtr detectBestCRC32C(){
        static const int SSE42_BIT = 20;
        unsigned ecx = cpuid(1);
        bool hasSSE42 = ecx & (1 << SSE42_BIT);
        if (hasSSE42) {
            std::cout << "using hardware base sse computation" << std::endl;
            return &IteratorbasedCRC32::SSEBasedCRC32; //crc32 hardware accelarated;
        } else {
            std::cout << "using software base sse computation" << std::endl;
            return &IteratorbasedCRC32::SoftwareBasedCRC32; //crc32cSlicingBy8;
        }
    }
    CRC32CFunctionPtr crcFunction;
public:
    IteratorbasedCRC32(): crc(0) {
        crcFunction = detectBestCRC32C();
    }

    virtual ~IteratorbasedCRC32() {};

    unsigned operator()( ContainerT_iterator iter, const ContainerT_iterator end) {
        unsigned crc = 0;
        while(iter != end) {
            char * data = reinterpret_cast<char*>(&(*iter) );
            crc =((*this).*(crcFunction))(data, sizeof(typename ContainerT::value_type*), crc);
            ++iter;
        }
        return crc;
    }
};

#endif /* ITERATORBASEDCRC32_H_ */
