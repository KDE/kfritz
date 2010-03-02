/*
 * libfritz++
 *
 * Copyright (C) 2007-2010 Joachim Wilke <libfritz@joachim-wilke.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <string.h>
#include <algorithm>
#include <sstream>
#include "FritzFonbook.h"
#include "FritzClient.h"
#include "Tools.h"
#include <TcpClient++.h>
#include "Config.h"

namespace fritz {

// this array contains characters encoded with ISO-8859-15, take care when editing this file
const char *Entities[97][2] = {
	{"&nbsp;",  " "},
	{"&iexcl;", "�"},
	{"&cent;",  "�"},
	{"&pound;", "�"},
	{"&curren;","�"},
	{"&yen;",   "�"},
	{"&brvbar;","�"},
	{"&sect;",  "�"},
	{"&uml;",   "�"},
	{"&copy;",  "�"},
	{"&ordf;",  "�"},
	{"&laquo;", "�"},
	{"&not;",   "�"},
	{"&shy;",   "�"},
	{"&reg;",   "�"},
	{"&macr;",  "�"},
	{"&deg;",   "�"},
	{"&plusmn;","�"},
	{"&sup2;",  "�"},
	{"&sup3;",  "�"},
	{"&acute;", "�"},
	{"&micro;", "�"},
	{"&para;",  "�"},
	{"&middot;","�"},
	{"&cedil;", "�"},
	{"&sup1;",  "�"},
	{"&ordm;",  "�"},
	{"&raquo;", "�"},
	{"&frac14;","�"},
	{"&frac12;","�"},
	{"&frac34;","�"},
	{"&iquest;","�"},
	{"&Agrave;","�"},
	{"&Aacute;","�"},
	{"&Acirc;", "�"},
	{"&Atilde;","�"},
	{"&Auml;",  "�"},
	{"&Aring;", "�"},
	{"&AElig;", "�"},
	{"&Ccedil;","�"},
	{"&Egrave;","�"},
	{"&Eacute;","�"},
	{"&Ecirc;", "�"},
	{"&Euml;",  "�"},
	{"&Igrave;","�"},
	{"&Iacute;","�"},
	{"&Icirc;", "�"},
	{"&Iuml;",  "�"},
	{"&ETH;",   "�"},
	{"&Ntilde;","�"},
	{"&Ograve;","�"},
	{"&Oacute;","�"},
	{"&Ocirc;", "�"},
	{"&Otilde;","�"},
	{"&Ouml;",  "�"},
	{"&times;", "�"},
	{"&Oslash;","�"},
	{"&Ugrave;","�"},
	{"&Uacute;","�"},
	{"&Ucirc;", "�"},
	{"&Uuml;",  "�"},
	{"&Yacute;","�"},
	{"&THORN;", "�"},
	{"&szlig;", "�"},
	{"&agrave;","�"},
	{"&aacute;","�"},
	{"&acirc;", "�"},
	{"&atilde;","�"},
	{"&auml;",  "�"},
	{"&aring;", "�"},
	{"&aelig;", "�"},
	{"&ccedil;","�"},
	{"&egrave;","�"},
	{"&eacute;","�"},
	{"&ecirc;", "�"},
	{"&euml;",  "�"},
	{"&igrave;","�"},
	{"&iacute;","�"},
	{"&icirc;", "�"},
	{"&iuml;",  "�"},
	{"&eth;",   "�"},
	{"&ntilde;","�"},
	{"&ograve;","�"},
	{"&oacute;","�"},
	{"&ocirc;", "�"},
	{"&otilde;","�"},
	{"&ouml;",  "�"},
	{"&divide;","�"},
	{"&oslash;","�"},
	{"&ugrave;","�"},
	{"&uacute;","�"},
	{"&ucirc;", "�"},
	{"&uuml;",  "�"},
	{"&yacute;","�"},
	{"&thorn;", "�"},
	{"&yuml;",  "�"},
	{"&amp;",   "&"},
};

std::string convertEntities(std::string s) {
	if (s.find("&") != std::string::npos) {
		// convert the entities from iso-8859-15 to current system character table
		CharSetConv *conv = new CharSetConv("ISO-8859-15", CharSetConv::SystemCharacterTable());

		for (int i=0; i<97; i++) {
			std::string::size_type pos = s.find(Entities[i][0]);
			if (pos != std::string::npos) {
				s.replace(pos, strlen(Entities[i][0]), conv->Convert(Entities[i][1]));
				i--; //search for the same entity again
			}
		}
		delete (conv);
	}
	return s;
}

FritzFonbook::FritzFonbook()
:PThread("FritzFonbook")
{
	title = I18N_NOOP("Fritz!Box phone book");
	techId = "FRITZ";
	displayable = true;
	setInitialized(false);
}

FritzFonbook::~FritzFonbook() {
	// don't delete the object, while the thread is still active
	while (Active())
		pthread::CondWait::SleepMs(100);
}

bool FritzFonbook::Initialize() {
	return Start();
}

void FritzFonbook::Action() {
	setInitialized(false);
	fonbookList.clear();

	FritzClient fc;
	std::string msg = fc.RequestFonbook();

	if (msg.find("<?xml") == std::string::npos)
		ParseHtmlFonbook(&msg);
	else
		ParseXmlFonbook(&msg);

	INF("read " << fonbookList.size() << " entries.");
	setInitialized(true);

	std::sort(fonbookList.begin(), fonbookList.end());
}

void FritzFonbook::ParseHtmlFonbook(std::string *msg) {
	DBG("Parsing fonbook using html parser.")
	// determine charset (default for old firmware versions is iso-8859-15)
	size_t pos, p1, p2;
	std::string charset = "ISO-8859-15";
	pos = msg->find("<meta http-equiv=content-type");
	if (pos != std::string::npos) {
		pos = msg->find("charset=", pos);
		if (pos != std::string::npos)
			charset = msg->substr(pos+8, msg->find('"', pos)-pos-8);
	}
	DBG("using charset " << charset);

	CharSetConv *conv = new CharSetConv(charset.c_str(), CharSetConv::SystemCharacterTable());
	const char *s_converted = conv->Convert(msg->c_str());
	std::string msgConv = s_converted;
	delete (conv);

	// parse answer
	pos = 0;
	int count = 0;
	// parser for old format
	const std::string tag("(TrFon(");
	while ((p1 = msgConv.find(tag, pos)) != std::string::npos) {
		p1 += 7; // points to the first "
		int nameStart     = msgConv.find(',', p1)          +3;
		int nameStop      = msgConv.find('"', nameStart)   -1;
		int numberStart   = msgConv.find(',', nameStop)    +3;
		int numberStop    = msgConv.find('"', numberStart) -1;
		if (msgConv[nameStart] == '!') // skip '!' char, older firmware versions use to mark VIPs
			nameStart++;
		std::string namePart = msgConv.substr(nameStart, nameStop - nameStart+1);
		std::string namePart2 = convertEntities(namePart);
		std::string numberPart = msgConv.substr(numberStart, numberStop - numberStart+1);
		if (namePart2.length() && numberPart.length()) {
			FonbookEntry fe(namePart2, numberPart, FonbookEntry::TYPE_NONE);
			fonbookList.push_back(fe);
			//DBG("(%s / %s)", fe.number.c_str(), fe.name.c_str());
		}
		pos = p1+10;
		count++;
	}
	// parser for new format
	pos = 0;
	const std::string tag2("TrFonName(");
	const std::string tag3("TrFonNr("	);
	while ((p2 = msgConv.find(tag3, pos)) != std::string::npos) {
		int typeStart     = p2 + 9;
		int numberStart   = msgConv.find(',', p2)    +3;
		int typeStop      = numberStart - 5;
		int numberStop    = msgConv.find('"', numberStart) -1;
		p1 = msgConv.rfind(tag2, p2);
		p1 += 7; // points to the first "
		int nameStart     = msgConv.find(',', p1)          +3;
		int nameStop      = msgConv.find('"', nameStart)   -1;
		std::string namePart   = msgConv.substr(nameStart, nameStop - nameStart+1);
		std::string namePart2  = convertEntities(namePart);
		std::string numberPart = msgConv.substr(numberStart, numberStop - numberStart+1);

		std::string typePart   = msgConv.substr(typeStart, typeStop - typeStart+1);
		FonbookEntry::eType type = FonbookEntry::TYPE_NONE;
		if      (typePart.compare("home") == 0)
			type = FonbookEntry::TYPE_HOME;
		else if (typePart.compare("mobile") == 0)
			type = FonbookEntry::TYPE_MOBILE;
		else if (typePart.compare("work") == 0)
			type = FonbookEntry::TYPE_WORK;

		if (namePart2.length() && numberPart.length()) {
			FonbookEntry fe(namePart2, numberPart, type);
			fonbookList.push_back(fe);
			//DBG("(%s / %s / %i)", fe.number.c_str(), fe.name.c_str(), fe.type);
		}
		pos = p2+10;
		count++;
	}
}

std::string FritzFonbook::ExtractXmlAttributeValue(std::string element, std::string attribute, std::string xml) {
	size_t posStart = xml.find("<"+element);
	if (posStart != std::string::npos) {
		posStart = xml.find(attribute+"=\"", posStart);
		if (posStart != std::string::npos) {
			size_t posEnd = xml.find("\"", posStart + attribute.length() + 2);
			if (posEnd != std::string::npos)
				return xml.substr(posStart + attribute.length() + 2, posEnd - posStart - attribute.length() - 2);
		}
	}
	return "";
}

std::string FritzFonbook::ExtractXmlElementValue(std::string element, std::string xml) {
	size_t posStart = xml.find("<"+element);
	if (posStart != std::string::npos) {
		posStart = xml.find(">", posStart);
		size_t posEnd   = xml.find("</"+element+">");
		if (posEnd != std::string::npos)
			return xml.substr(posStart + 1, posEnd - posStart - 1);
	}
	return "";
}

void FritzFonbook::ParseXmlFonbook(std::string *msg) {
	DBG("Parsing fonbook using xml parser.")
	// determine charset
	size_t pos, posStart, posEnd;
	std::string charset = "ISO-8859-15";
	posStart = msg->find("encoding=\"");
	if (posStart != std::string::npos) {
		posEnd = msg->find("\"", posStart + 10);
		if (posEnd != std::string::npos)
			charset = msg->substr(posStart + 10, posEnd - posStart - 10);
	}
	DBG("using charset " << charset);

	CharSetConv *conv = new CharSetConv(charset.c_str(), CharSetConv::SystemCharacterTable());
	const char *s_converted = conv->Convert(msg->c_str());
	std::string msgConv = s_converted;
	delete (conv);

	pos = msgConv.find("<contact>");
	while (pos != std::string::npos) {
		std::string msgPart = msgConv.substr(pos, msgConv.find("</contact>", pos) - pos + 10);
		std::string category = ExtractXmlElementValue("category", msgPart);
		std::string name     = convertEntities(ExtractXmlElementValue("realName", msgPart));
		size_t posNumber = msgPart.find("<number");
		while (posNumber != std::string::npos) {
			std::string msgPartofPart = msgPart.substr(posNumber, msgPart.find("</number>", posNumber) - posNumber + 9);
			std::string number    = ExtractXmlElementValue  ("number",              msgPartofPart);
			std::string typeStr   = ExtractXmlAttributeValue("number", "type",      msgPartofPart);
			std::string quickdial = ExtractXmlAttributeValue("number", "quickdial", msgPartofPart);
			std::string vanity    = ExtractXmlAttributeValue("number", "vanity",    msgPartofPart);
			std::string prio      = ExtractXmlAttributeValue("number", "prio",      msgPartofPart);

			if (number.size()) { // the xml may contain entries without a number!
				FonbookEntry::eType type = FonbookEntry::TYPE_NONE;
				if (typeStr == "home")
					type = FonbookEntry::TYPE_HOME;
				if (typeStr == "mobile")
					type = FonbookEntry::TYPE_MOBILE;
				if (typeStr == "work")
					type = FonbookEntry::TYPE_WORK;

				FonbookEntry fe(name, number, type, \
						        (category == "1") ? true : false, quickdial, vanity, atoi(prio.c_str()));
				fonbookList.push_back(fe);
			}
			posNumber = msgPart.find("<number", posNumber+1);
		}

		pos = msgConv.find("<contact>", pos+1);
	}
}

void FritzFonbook::Reload() {
	this->Start();
}

}
