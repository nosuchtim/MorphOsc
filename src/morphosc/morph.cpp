#if 0
#include "Morph.h"

#include <map>

Morph::Morph(cJSON* config, CursorListener *listener) {

	ofLog(OF_LOG_NOTICE, "SenselDeviceList num_devices=%d", m_senselDeviceList.num_devices);

	m_cursorlistener = listener;

	if (senselGetDeviceList(&m_senselDeviceList) == SENSEL_ERROR) {
		ofLogError("Error in senselGetDeviceList, no Sensel devices detected?");
	}
	if (m_senselDeviceList.num_devices == 0) {
		fprintf(stdout, "No Sensel devices found!\n");
		return;
	}

	// If no explicit serialmap is given, we create one.
	if (m_serialmap.size() == 0) {
		for (int i = 0; i < m_senselDeviceList.num_devices; i++) {
			SenselDeviceID& dev = m_senselDeviceList.devices[i];
			m_serialmap.insert(std::pair<unsigned char*, int>(
				dev.serial_num, m_sidinitial + i*m_sidincrement)
			);
		}
	}

	for (auto& x : m_serialmap) {
		SENSEL_HANDLE h;
		unsigned char* serial = x.first;
		if (senselOpenDeviceBySerialNum(&h, serial) == SENSEL_OK) {
			m_morphs.push_back(new OneMorph(h, serial, x.second));
		}
		else {
			fprintf(stdout, "Unable to find Morph with serial number '%s'\n", serial);
		}
	}
}

bool
Morph::init()
{
	if (m_morphs.size() == 0) {
		fprintf(stdout, "No Morphs have been found!\n");
		return false;
	}
	bool sensel_sensor_opened = false;

	for (auto& morph : m_morphs) {
		SENSEL_HANDLE h = morph->_handle;
		senselSetFrameContent(h, FRAME_CONTENT_CONTACTS_MASK);
		senselAllocateFrameData(h, &morph->_frame);
		senselStartScanning(h);
		for (int led = 0; led < 16; led++) {
			senselSetLEDBrightness(h, led, 0); //turn off LED
		}
	}

	return true;
}

void Morph::pressed(float x, float y, int sid, int cid, float force) {
	Cursor* c = new Cursor(sid, NosuchPos(x, y, force));
	// 	x,y,sid,cid);
	// m_cursorlistener->processCursor(,CURSOR_DOWN)
	// c->setForce(force);

	// std::list<TuioCursor*> cursorList = server->getTuioCursors();
}

void Morph::dragged(float x, float y, int sid, int cid, float force) {
#if 0
	TuioCursor *match = NULL;
	std::list<TuioCursor*> cursorList = server->getTuioCursors();
	// XXX - use auto here
	for (std::list<TuioCursor*>::iterator tuioCursor = cursorList.begin(); tuioCursor!=cursorList.end(); tuioCursor++) {
		if (((*tuioCursor)->getSessionID()) == sid) {
			match = (*tuioCursor);
			break;
		}
	}
	if ( match == NULL ) {
		// An early firmware bug (since fixed) produced occasional dragged messages after a release,
		// so we just ignore them until we get the next pressed message.
		fprintf(stderr, "Warning, drag message after release has been ignored!\n");
		if (server->verbose > 1) {
			fprintf(stdout, "DRAG_IGNORED sid=%d\n", sid);
		}
		// match = server->addTuioCursorId(x,y,sid,cid);
	} else {
		server->updateTuioCursor(match,x,y);
		match->setForce(force);
	}
#endif
}

void Morph::released(float x, float y, int sid, int cid, float force) {
#if 0
	// printf("released  uid=%d id=%d\n",uid,id);
	if (server->verbose > 1) {
		fprintf(stdout, "RELEASED          sid=%d\n", sid);
	}
	std::list<TuioCursor*> cursorList = server->getTuioCursors();
	TuioCursor *match = NULL;
	// XXX - use auto here
	for (std::list<TuioCursor*>::iterator tuioCursor = cursorList.begin(); tuioCursor!=cursorList.end(); tuioCursor++) {
		if (((*tuioCursor)->getSessionID()) == sid) {
			match = (*tuioCursor);
			break;
		}
	}
	if (match!=NULL) {
		server->removeTuioCursor(match);
	}
#endif
}

void
Morph::listdevices() {
	SenselDeviceList list;

	senselGetDeviceList(&list);
	if (list.num_devices == 0) {
		fprintf(stdout, "No Sensel devices found!\n");
	}

	for (int i = 0; i < list.num_devices; i++) {
		SenselDeviceID& dev = list.devices[i];
		fprintf(stdout, "Sensel Morph device port=%s idx=%d serialnum=%s\n", dev.com_port, dev.idx, dev.serial_num);
	}
}

#define MAX_MORPHS 16

void Morph::run() {

	for (auto& morph : m_morphs) {
			SENSEL_HANDLE h = morph->_handle;
			SenselFrameData* frame = morph->_frame;
			senselReadSensor(h);
			unsigned int num_frames = 0;
			senselGetNumAvailableFrames(h, &num_frames);
			for (unsigned int f = 0; f < num_frames; f++) {
				senselGetFrame(h, frame);
#if 0
				if (server->verbose > 1 && frame->n_contacts > 0) {
					printf("FRAME f=%d n_contacts=%d\n", f, frame->n_contacts);
				}
#endif
				for (int i = 0; i < frame->n_contacts; i++) {

					unsigned int state = frame->contacts[i].state;
					char* statestr =
						state == CONTACT_INVALID ? "CONTACT_INVALID" :
						state == CONTACT_START ? "CONTACT_START" :
						state == CONTACT_MOVE ? "CONTACT_MOVE" :
						state == CONTACT_END ? "CONTACT_END" :
						"UNKNOWN_CONTACT_STATE";

					SenselContact& c = frame->contacts[i];
					int sid = morph->_initialsid + c.id;
					unsigned char cid = c.id;
					float force = c.total_force;
					float x_mm = c.x_pos;
					float y_mm = c.y_pos;
					//Read out shape information (ellipses)
					float major = c.major_axis;
					float minor = c.minor_axis;
					float orientation = c.orientation;

					// ofLog(OF_LOG_NOTICE,"Serial: %s   Contact ID: %d   Session ID: %d   State: %s   xy=%.4f,%.4f\n",
					// 		morph->_serialnum, cid, sid, statestr, x_mm, y_mm);
		
					float x_norm = x_mm / MORPH_WIDTH;
					float y_norm = y_mm / MORPH_HEIGHT;
					float f_norm = force / MORPH_MAX_FORCE;
		
					char* event = "unknown";
					switch (c.state)
					{
					case CONTACT_START:
						event = "start";
						// senselSetLEDBrightness(h,cid, 100); //turn on LED
						// pressed(x_norm, y_norm, sid, cid, f_norm);
						break;

					case CONTACT_MOVE:
						event = "move";
						// dragged(x_norm, y_norm, sid, cid, f_norm);
						break;

					case CONTACT_END:
						event = "end";
						// senselSetLEDBrightness(h,cid, 0); //turn LED off
						// released(x_norm, y_norm, sid, cid, f_norm);
						break;

					case CONTACT_INVALID:
						event = "invalid";
						break;
					}
		
					// printf("TUIO  Contact ID %d, event=%s, mm coord: (%f, %f), force=%f, " \
					// 	"major=%f, minor=%f, orientation=%f\n",
					// 	id, event, x_mm, y_mm, force, major, minor, orientation);
				}
		
				// server->update();
			}
		}
}
#endif
