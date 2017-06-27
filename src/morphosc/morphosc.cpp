
#include <stdio.h>
#include <signal.h>
#include "xgetopt.h"
#include "tchar.h"
#include "morphosc.h"

volatile sig_atomic_t ctrl_c_requested = false;
bool verbose = false;

int Morph::m_next_initial_id = 11000;
float MorphHandler::MaxForce = 1500.0;

void handle_ctrl_c(int sig)
{
	ctrl_c_requested = true;
}

void
printUsage() {
	printf("\n");
	printf("usage: morphosc[-v][-l]{ configfile }\n");
	printf("  -v               Enabled verbose output\n");
	printf("  -l               List all Morphs and their serial numbers\n");
	printf("  {configfile}     Config filename\n");
	printf("\n");
}

int main(int argc, char **argv)
{
	const char *host;
	int port = -1;
	int c;
	bool flipx = false;
	bool flipy = false;
	bool listdevices = false;
	std::string configfilename;

	signal(SIGINT, handle_ctrl_c);

	while ((c = getopt(argc, (const char**)argv, "vl")) != EOF) {
		switch (c) {
		case _T('v'):
			verbose = true;
			break;
		case _T('l'):
			listdevices = true;
			break;
		case _T('?'):
			printUsage();
			return 1;
		}
	}

	if (listdevices) {
		MorphHandler::listdevices();
		return 0;
	}

	// The configfile is a required argument
	if ((argc-optind) != 1) {
		printUsage();
		return 1;
	}

	configfilename = argv[argc - 1];

	std::string err;
	cJSON* config = jsonReadFile(configfilename,err);

	if (config == NULL) {
		printf("Unable to read configfile '%s'\n",configfilename.c_str());
		exit(1);
	}

	cJSON* h = cJSON_GetObjectItem(config, "host");
	if ( h == NULL ) {
		host = "3333@127.0.0.1";
	}
	else {
		host = h->valuestring;
	}

	const char* amp = strchr(host, '@');
	if (amp) {
		port = atoi(host);
		host = amp + 1;
	} else {
		port = 3333;
	}

	cJSON* maxforce = cJSON_GetObjectItem(config, "maxforce");
	if (maxforce) {
		MorphHandler::MaxForce = (float) maxforce->valuedouble;
	}

	printf("Sending OSC to host=%s port=%d\n", host, port);

	OscSender* sender = new OscSender(host,port);
	MorphHandler *morph = new MorphHandler(config, sender);

	while ( !ctrl_c_requested ) {
		morph->run();
	}

	delete(morph);

	return 0;
}

OscSender::OscSender(const char* host, int port) {
	m_outputstream = new osc::OutboundPacketStream(m_buffer, IP_MTU_SIZE);
	m_host = new IpEndpointName(host, port);
	m_socket = new UdpTransmitSocket(*m_host);
}

void
OscSender::SendContact(MorphControl mc, float value) {
	m_outputstream->Clear();
	// The value coming in is 0.0 to 1.0, and we scale it to m_min/m_max
	value = mc.m_min + value * mc.m_max;
	*m_outputstream << osc::BeginMessage(mc.m_osc.c_str()) << value << osc::EndMessage;
	m_socket->Send(m_outputstream->Data(), m_outputstream->Size());
}

Morph::Morph(std::string serialnum, cJSON* config) {

	SenselStatus status;

	m_serialnum = serialnum;
	m_initial_id = m_next_initial_id;
	m_next_initial_id += 1000;
	unsigned char* sn = (unsigned char*) serialnum.c_str();

	// If the serial number is just "0", "1", "2", etc, open by the list index rather than serialnum
	if (isdigit(sn[0]) && sn[1] == '\0') {
		unsigned char id = sn[0] - '0';
		status = senselOpenDeviceByID(&m_handle, id);
	} else {
		status = senselOpenDeviceBySerialNum(&m_handle, sn);
	}

	if (status == SENSEL_OK) {

		m_x = MorphControl(cJSON_GetObjectItem(config,"x"));
		m_y = MorphControl(cJSON_GetObjectItem(config,"y"));
		m_force = MorphControl(cJSON_GetObjectItem(config,"force"));
		m_flipX = false;
		m_flipY = false;

		senselSetFrameContent(m_handle, FRAME_CONTENT_CONTACTS_MASK);
		senselAllocateFrameData(m_handle, &m_frame);
		senselStartScanning(m_handle);
		for (int led = 0; led < 16; led++) {
			senselSetLEDBrightness(m_handle, led, 0); //turn off LED
		}
	} else {
		printf("Unable to open serialnum=%s\n", serialnum.c_str());
		m_handle = 0;
	}
}

MorphHandler::MorphHandler(cJSON* config, OscSender *sender) {

	m_oscsender = sender;

	if (senselGetDeviceList(&m_senselDeviceList) == SENSEL_ERROR) {
		printf("Error in senselGetDeviceList, no Sensel devices detected?");
		exit(1);
	}
	if (m_senselDeviceList.num_devices == 0) {
		printf("No Sensel devices found!\n");
		exit(1);
	}

	cJSON* morphs = cJSON_GetObjectItem(config, "morphs");
	if (morphs == NULL) {
		printf("No 'morphs' entry in JSON\n");
		exit(1);
	}
	for (cJSON* j = morphs->child; j != NULL; j = j->next) {
		std::string serialnum = std::string(j->string);
		if (j->type != cJSON_Object) {
			printf("Didn't get expected object in JSON for serialnum %s\n", serialnum.c_str());
			exit(1);
		}

		// Special handling of * as a serial number, opens all devices
		if (serialnum == "*") {
			for (int i = 0; i < m_senselDeviceList.num_devices; i++) {
				std::string sn = std::string((const char*) m_senselDeviceList.devices[i].serial_num);
				Morph* morph = new Morph(sn,j);
				m_morphs.push_back(morph);
			}
		}
		else {
			Morph* morph = new Morph(serialnum, j);
			if (morph == NULL) {
				printf("Unable to instantiate Morph for serialnum%s\n", j->valuestring);
				exit(1);
			}
			m_morphs.push_back(morph);
		}
	}
}

void MorphHandler::processContact(Morph* m, int id, float x, float y, float force) {
	Cursor* c = new Cursor(id, NosuchPos(x, y, force));
	if ( x >= 0.0 ) {
		m_oscsender->SendContact(m->m_x, x);
	}
	if (y >= 0.0) {
		m_oscsender->SendContact(m->m_y, y);
	}
	if ( force >= 0.0 ) {
		m_oscsender->SendContact(m->m_force, force);
	}
}

void
MorphHandler::listdevices() {
	SenselDeviceList list;

	senselGetDeviceList(&list);
	if (list.num_devices == 0) {
		printf( "No Sensel devices found!\n");
	}

	for (int i = 0; i < list.num_devices; i++) {
		SenselDeviceID& dev = list.devices[i];
		printf( "Sensel Morph device port=%s idx=%d serialnum=%s\n", dev.com_port, dev.idx, dev.serial_num);
	}
}

#define MAX_MORPHS 16

void MorphHandler::run() {

	for (auto& morph : m_morphs) {
		SENSEL_HANDLE h = morph->m_handle;
		SenselFrameData* frame = morph->m_frame;
		senselReadSensor(h);
		unsigned int num_frames = 0;
		senselGetNumAvailableFrames(h, &num_frames);
		for (unsigned int f = 0; f < num_frames; f++) {
			senselGetFrame(h, frame);
			for (int i = 0; i < frame->n_contacts; i++) {

				SenselContact& c = frame->contacts[i];
				int cid = morph->m_initial_id + c.id;
				float force = c.total_force;
				float x_mm = c.x_pos;
				float y_mm = c.y_pos;
				//Read out shape information (ellipses)
				float major = c.major_axis;
				float minor = c.minor_axis;
				float orientation = c.orientation;

				float x_norm = x_mm / MORPH_WIDTH;
				float y_norm = y_mm / MORPH_HEIGHT;

				y_norm = 1.0f - y_norm;  // to get y=0 at bottom of pad

				if (force > MorphHandler::MaxForce) {
					printf("Warning: Morph::MaxForce is %f, but received force of %f\n",MorphHandler::MaxForce,force);
					force = MorphHandler::MaxForce;
				}
				float force_norm = force / MorphHandler::MaxForce;

				char* event = "unknown";
				switch (c.state)
				{
				case CONTACT_START:
					event = "CONTACT_START";
					processContact(morph, cid, x_norm, y_norm, force_norm);
					break;

				case CONTACT_MOVE:
					event = "CONTACT_MOVE";
					processContact(morph, cid, x_norm, y_norm, force_norm);
					break;

				case CONTACT_END:
					event = "CONTACT_END";
					processContact(morph, cid, -1.0, -1.0, 0.0);
					break;

				case CONTACT_INVALID:
					event = "CONTACT_INVALID";
					break;
				}

				if (verbose) {
					printf("%s %s cid=%d x=%f y=%f force=%f\n",morph->m_serialnum.c_str(),event,cid,x_norm,y_norm,force_norm);
				}
			}
		}
	}
}
