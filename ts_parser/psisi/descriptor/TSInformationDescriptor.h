#ifndef _TS_INFORMATION_DESCRIPTOR_H_
#define _TS_INFORMATION_DESCRIPTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "Defs.h"
#include "TsCommon.h"
#include "Descriptor.h"


class CTSInformationDescriptor : public CDescriptor
{
public:
	class CTransmission {
	public:
		class CService {
		public:
			CService (void)
				:service_id (0)
			{}
			virtual ~CService (void) {}

			uint16_t service_id;
		};
	public:
		CTransmission (void)
			:transmission_type_info (0)
			,num_of_service (0)
		{
			services.clear();
		}
		virtual ~CTransmission (void) {}

		uint8_t transmission_type_info;
		uint8_t num_of_service;
		std::vector <CService> services;
	};
public:
	explicit CTSInformationDescriptor (const CDescriptor &obj);
	virtual ~CTSInformationDescriptor (void);

	void dump (void) const override;

	uint8_t remote_control_key_id;
	uint8_t length_of_ts_name;
	uint8_t transmission_type_count;
	uint8_t ts_name_char [0xff];
	std::vector <CTransmission> transmissions;
	// reserve

private:
	bool parse (void);

};

#endif
