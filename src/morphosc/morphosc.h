#pragma once

#include <list>
#include <map>
#include <math.h>
#include "NosuchUtil.h"
#include "sensel.h"
#include "sensel_protocol.h"
#include "sensel_device.h"

#include "osc/OscOutboundPacketStream.h"
#include "ip/UdpSocket.h"

#define MORPH_WIDTH 230
#define MORPH_HEIGHT 130
#define MAX_IGESTURE_ID 12

class NosuchPos {
public:
	NosuchPos() {
		set(0.0, 0.0, 0.0);
	}
	NosuchPos(float xx, float yy, float zz) {
		set(xx, yy, zz);
	};
	void set(float xx, float yy, float zz) {
		x = xx;
		y = yy;
		z = zz;
	}

	float x;
	float y;
	float z;
};

class Cursor
{
public:
	// methods
	Cursor(int sid, NosuchPos pos) {
		m_sid = sid;
		m_pos = pos;
	}
	NosuchPos m_pos;
	int m_sid;
};

#define CURSOR_DOWN 0
#define CURSOR_DRAG 1
#define CURSOR_UP 2
#define IP_MTU_SIZE 1536

class MorphControl {
public:
	MorphControl() {
		m_osc = "";
		m_min = 0.0;
		m_max = 1.0;
	}
	MorphControl(cJSON* j) {

		cJSON* j_osc = cJSON_GetObjectItem(j, "osc");
		if (j_osc == NULL || j_osc->type != cJSON_String) {
			printf("Bad value for 'osc'\n");
			exit(1);
		}
		m_osc = std::string(j_osc->valuestring);

		cJSON* j_min = cJSON_GetObjectItem(j, "min");
		if (j_min == NULL || j_min->type != cJSON_Number) {
			printf("Bad value for 'min'\n");
			exit(1);
		}
		m_min = (float) j_min->valuedouble;

		cJSON* j_max = cJSON_GetObjectItem(j, "max");
		if (j_max == NULL || j_max->type != cJSON_Number) {
			printf("Bad value for 'max'\n");
			exit(1);
		}
		m_max = (float) j_max->valuedouble;
	}

	std::string m_osc;
	float m_min;
	float m_max;
};

class Morph {
public:
	Morph(std::string serial, cJSON* config);

	SENSEL_HANDLE m_handle;
	std::string m_serialnum;
	SenselFrameData* m_frame;
	MorphControl m_x;
	MorphControl m_y;
	MorphControl m_z;
	int m_initial_id;
	bool m_flipX;
	bool m_flipY;

private:
	static int m_next_initial_id;
};


class OscSender {
public:
	OscSender(const char* host, int port);
	void SendContact(MorphControl mc, float value);
private:
	char m_buffer[IP_MTU_SIZE];
	osc::OutboundPacketStream* m_outputstream;
	UdpTransmitSocket* m_socket;
	const IpEndpointName* m_host;
};

class MorphHandler { 
	
public:
	MorphHandler(cJSON* config, OscSender* sender);
	~MorphHandler() {
	};
	
	static void listdevices();

	void run();
	bool init();
	void processContact(Morph* m, int id, float x, float y, float z);

	static float MaxForce;

private:

	SenselDeviceList m_senselDeviceList;
	OscSender* m_oscsender;
	std::list<Morph*> m_morphs;

	// int width, height;
};
