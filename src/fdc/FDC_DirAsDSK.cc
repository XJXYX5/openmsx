// $Id$



#include "File.hh"
#include "FDC_DirAsDSK.hh"
#include "FileContext.hh"

#include <algorithm>
#include <cassert>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef	NO_BZERO
#define	bzero(x,y)	memset(x,0,y)
#endif

using std::transform;

/* macros to change DirEntries */
#define setsh(x,y) {x[0]=y;x[1]=y>>8;}
#define setlg(x,y) {x[0]=y;x[1]=y>>8;x[2]=y>>16;x[3]=y>>24;}

/* macros to read DirEntries */
#define rdsh(x) (x[0]+(x[1]<<8))
#define rdlg(x) (x[0]+(x[1]<<8)+(x[2]<<16)+(x[3]<<24))

namespace openmsx {

#define EOF_FAT 0xFFF /* signals EOF in FAT */
#define NODIRENTRY    4000
#define CACHEDCLUSTER 4001

//Bootblock taken from a philips  nms8250 formatted disk
const string FDC_DirAsDSK::BootBlockFileName = ".sector.boot";
const byte FDC_DirAsDSK::DefaultBootBlock[] =
{
0xeb,0xfe,0x90,0x4e,0x4d,0x53,0x20,0x32,0x2e,0x30,0x50,0x00,0x02,0x02,0x01,0x00,
0x02,0x70,0x00,0xa0,0x05,0xf9,0x03,0x00,0x09,0x00,0x02,0x00,0x00,0x00,0xd0,0xed,
0x53,0x59,0xc0,0x32,0xd0,0xc0,0x36,0x56,0x23,0x36,0xc0,0x31,0x1f,0xf5,0x11,0xab,
0xc0,0x0e,0x0f,0xcd,0x7d,0xf3,0x3c,0xca,0x63,0xc0,0x11,0x00,0x01,0x0e,0x1a,0xcd,
0x7d,0xf3,0x21,0x01,0x00,0x22,0xb9,0xc0,0x21,0x00,0x3f,0x11,0xab,0xc0,0x0e,0x27,
0xcd,0x7d,0xf3,0xc3,0x00,0x01,0x58,0xc0,0xcd,0x00,0x00,0x79,0xe6,0xfe,0xfe,0x02,
0xc2,0x6a,0xc0,0x3a,0xd0,0xc0,0xa7,0xca,0x22,0x40,0x11,0x85,0xc0,0xcd,0x77,0xc0,
0x0e,0x07,0xcd,0x7d,0xf3,0x18,0xb4,0x1a,0xb7,0xc8,0xd5,0x5f,0x0e,0x06,0xcd,0x7d,
0xf3,0xd1,0x13,0x18,0xf2,0x42,0x6f,0x6f,0x74,0x20,0x65,0x72,0x72,0x6f,0x72,0x0d,
0x0a,0x50,0x72,0x65,0x73,0x73,0x20,0x61,0x6e,0x79,0x20,0x6b,0x65,0x79,0x20,0x66,
0x6f,0x72,0x20,0x72,0x65,0x74,0x72,0x79,0x0d,0x0a,0x00,0x00,0x4d,0x53,0x58,0x44,
0x4f,0x53,0x20,0x20,0x53,0x59,0x53,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

// read FAT-entry from FAT in memory
word FDC_DirAsDSK::ReadFAT(word clnr)
{ 
  byte *P = FAT + (clnr * 3) / 2;
  return (clnr & 1) ?
         (P[0] >> 4) + (P[1] << 4) :
	 P[0] + ((P[1] & 0x0F) << 8);
}

// write an entry to FAT in memory
void FDC_DirAsDSK::WriteFAT(word clnr, word val)
{
	byte* P=FAT + (clnr * 3) / 2;
	if (clnr & 1) { 
		P[0] = (P[0] & 0x0F) + (val << 4);
		P[1] = val >> 4;
	} else {
		P[0] = val;
		P[1] = (P[1] & 0xF0) + ((val >> 8) & 0x0F);
	}
}

int FDC_DirAsDSK::findFirstFreeCluster()
{
	int cluster=2;
	while ((cluster <= MAX_CLUSTER) && ReadFAT(cluster)) {
		cluster++;
	};
	return cluster;
}

// check if a filename is used in the emulated MSX disk
bool FDC_DirAsDSK::checkMSXFileExists(const string& msxfilename)
{
	//TODO: complete this
	unsigned pos = msxfilename.find_last_of('/');
	string tmp;
	if (pos != string::npos) {
		tmp = msxfilename.substr(pos + 1);
	} else {
		tmp = msxfilename;
	}

	for (int i = 0; i < 112; i++) {
		if (strncmp((const char*)(mapdir[i].msxinfo.filename),
			    tmp.c_str(), 11) == 0 ) {
			return true;
		}
	}
	return false;
}

// check if a file is already mapped into the fake DSK
bool FDC_DirAsDSK::checkFileUsedInDSK(const string& fullfilename)
{
	for (int i = 0; i < 112; i++) {
		if (mapdir[i].filename == fullfilename) {
			return true;
		}
	}
	return false;
}

// create an MSX filename 8.3 format, if needed in vfat like abreviation
char toMSXChr(char a)
{
	a = ::toupper(a);
	if (a == ' ') {
		a = '_';
	}
	return a;
}
string FDC_DirAsDSK::makeSimpleMSXFileName(const string& fullfilename)
{
	unsigned pos = fullfilename.find_last_of('/');
	string tmp;
	if (pos != string::npos) {
		tmp = fullfilename.substr(pos + 1);
	} else {
		tmp = fullfilename;
	}
	PRT_DEBUG("filename before transform " << tmp);
	transform(tmp.begin(), tmp.end(), tmp.begin(), toMSXChr);
	PRT_DEBUG("filename after transform " << tmp);

	string file, ext;
	pos = tmp.find_last_of('.');
	if (pos != string::npos) {
		file = tmp.substr(0, pos);
		ext  = tmp.substr(pos + 1);
	} else {
		file = tmp;
		ext = "";
	}

	PRT_DEBUG("adding correct amount of spaces");
	file += "        ";
	ext  += "   ";
	file = file.substr(0, 8);
	ext  = ext.substr(0, 3);

	return file + ext;
}


FDC_DirAsDSK::FDC_DirAsDSK(FileContext &context, const string &fileName)
{
	// TODO reolve fileName in given context
	
	// Here we create the fake diskimages based upon the files that can be
	// found in the 'fileName' directory
	PRT_INFO("Creating FDC_DirAsDSK object");
	DIR* dir = opendir(fileName.c_str());

	if (dir == NULL ) {
		throw MSXException("Not a directory");
	}

	// store filename as chroot dir for the msx disk
	MSXrootdir=fileName;

	// First create structure for the fake disk
	
	nbSectors = 1440; // asume a DS disk is used
	sectorsPerTrack = 9;
	nbSides = 2;

	std::string tmpfilename(MSXrootdir);
	tmpfilename+="/"+BootBlockFileName ;

	// Assign default boot disk to this instance
	memcpy(BootBlock, DefaultBootBlock, SECTOR_SIZE);

	// try to read boot block from file
	struct stat fst;
	bool readBootBlockFromFile = false;

	if (stat(tmpfilename.c_str(), &fst) ==0 ) {
		if ( fst.st_size == SECTOR_SIZE ){
		readBootBlockFromFile=true;
		FILE* file = fopen(tmpfilename.c_str(), "rb");
		if (file) {
			PRT_INFO("reading bootblock from " << tmpfilename);
			//fseek(file,0,SEEK_SET);
			// Read boot block from file
			fread(BootBlock, 1, SECTOR_SIZE, file);
			fclose(file);
		}
		}
	}
	
	// Assign empty directory entries
	for (int i = 0; i < 112; i++) {
		memset(&mapdir[i].msxinfo, 0, sizeof(MSXDirEntry));
	}

	// Make a full clear FAT
	memset(FAT, 0, SECTOR_SIZE * SECTORS_PER_FAT);
	// for some reason the first 3bytes are used to indicate the end of a cluster, making the first available cluster nr 2
	// some sources say that this indicates the disk fromat and FAT[0]should 0xF7 for single sided disk, and 0xF9 for double sided disks
	// TODO: check this :-)
	FAT[0] = 0xF9;
	FAT[1] = 0xFF;
	FAT[2] = 0xFF;

	//clear the clustermap so that they all point to 'clean' sectors
	for (int i = 0; i < MAX_CLUSTER; i++) {
		clustermap[i].dirEntryNr=NODIRENTRY;
	};

	//read directory and fill the fake disk
	struct dirent* d = readdir(dir);
	while (d) {
		string name(d->d_name);
		PRT_DEBUG("reading name in dir :" << name);
		//TODO: if bootsector read from file we should skip this file
		if ( ! readBootBlockFromFile ){
			updateFileInDSK(fileName + '/' + name); // used here to add file into fake dsk
		} else if ( name != BootBlockFileName ) {
			updateFileInDSK(fileName + '/' + name); // used here to add file into fake dsk
		}
		d = readdir(dir);
	}
	closedir(dir);
}

FDC_DirAsDSK::~FDC_DirAsDSK()
{
	PRT_DEBUG("Destroying FDC_DirAsDSK object");
}

void FDC_DirAsDSK::read(byte track, byte sector, byte side,
                   int size, byte* buf)
{
	assert(size == SECTOR_SIZE);
	
	int logicalSector = physToLog(track, side, sector);
	if (logicalSector >= nbSectors) {
		throw NoSuchSectorException("No such sector");
	}
	PRT_DEBUG("Reading sector : " << logicalSector );
	if (logicalSector == 0) {
		//copy our fake bootsector into the buffer
		PRT_DEBUG("Reading boot sector");
		memcpy(buf, BootBlock, SECTOR_SIZE);
	} else if (logicalSector < (1 + 2 * SECTORS_PER_FAT)) {
		//copy correct sector from FAT
		PRT_DEBUG("Reading fat sector : " << logicalSector );

		// quick-and-dirty:
		// we check all files in the faked disk for altered filesize
		// remapping each fat entry to its direntry and do some bookkeeping
		// to avoid multiple checks will probably be slower then this 
		for (int i = 0; i < 112; i++) {
			if ( ! mapdir[i].filename.empty()) {
				checkAlterFileInDisk(i);
			}
		};

		logicalSector = (logicalSector - 1) % SECTORS_PER_FAT;
		memcpy(buf, FAT + logicalSector * SECTOR_SIZE, size);
	} else if (logicalSector < 14) {
		//create correct DIR sector 
		PRT_DEBUG("Reading dir sector : " << logicalSector );
		logicalSector -= (1 + 2 * SECTORS_PER_FAT);
		int dirCount = logicalSector * 16;
		for (int i = 0; i < 16; i++) {
			checkAlterFileInDisk(dirCount);
			memcpy(buf, &(mapdir[dirCount++].msxinfo), 32);
			buf += 32;
		}
	} else {
		PRT_DEBUG("Reading mapped sector : " << logicalSector );
		// else get map from sector to file and read correct block
		// folowing same numbering as FAT eg. first data block is cluster 2
		int cluster = (int)((logicalSector - 14) / 2) + 2; 
		PRT_DEBUG("Reading cluster " << cluster );
		if (clustermap[cluster].dirEntryNr == NODIRENTRY ) {
			//return an 'empty' sector
			// 0xE5 is the value used on the Philips VG8250
			memset(buf, 0xE5, SECTOR_SIZE  );
		} else if (clustermap[cluster].dirEntryNr == CACHEDCLUSTER ) {
			PRT_INFO("No cached cluster routine implemented yet");
		} else {
			// open file and read data
			int offset = clustermap[cluster].fileOffset + (logicalSector & 1) * SECTOR_SIZE;
			string tmp = mapdir[clustermap[cluster].dirEntryNr].filename;
			PRT_DEBUG("  Reading from file " << tmp );
			PRT_DEBUG("  Reading with offset " << offset );
			checkAlterFileInDisk(tmp);
			try {
			FILE* file = fopen(tmp.c_str(), "r");
			if (file) {
				fseek(file,offset,SEEK_SET);
				fread(buf, 1, SECTOR_SIZE, file);
				fclose(file);
			} 
			} catch (...){
				PRT_DEBUG("problems with file reading");
			}
			//if (!file || ferror(file)) {
			//	throw DiskIOErrorException("Disk I/O error");
			//}
		}
	}
}

void FDC_DirAsDSK::checkAlterFileInDisk(const string& fullfilename)
{
	for (int i = 0; i < 112; i++) {
		if (mapdir[i].filename == fullfilename) {
			checkAlterFileInDisk(i);
		}
	}
}

void FDC_DirAsDSK::checkAlterFileInDisk(const int dirindex)
{
	int fsize;
	// compute time/date stamps
	struct stat fst;
	bzero(&fst,sizeof(struct stat));
	stat(mapdir[dirindex].filename.c_str(), &fst);
	fsize = fst.st_size;

	if ( mapdir[dirindex].filesize != fsize ) {
		updateFileInDisk(dirindex);
	};
}
void FDC_DirAsDSK::updateFileInDisk(const int dirindex)
{
	PRT_INFO("updateFileInDisk : " << mapdir[dirindex].filename );
	// compute time/date stamps
	int fsize;
	struct stat fst;
	bzero(&fst,sizeof(struct stat));
	stat(mapdir[dirindex].filename.c_str(), &fst);
	struct tm mtim = *localtime(&(fst.st_mtime));
	int t = (mtim.tm_sec >> 1) + (mtim.tm_min << 5) +
		(mtim.tm_hour << 11);
	setsh(mapdir[dirindex].msxinfo.time, t);
	t = mtim.tm_mday + ((mtim.tm_mon + 1) << 5) +
	    ((mtim.tm_year + 1900 - 1980) << 9);
	setsh(mapdir[dirindex].msxinfo.date, t);
	fsize = fst.st_size;

	mapdir[dirindex].filesize=fsize;
	int curcl = 2;
	curcl=rdsh(mapdir[dirindex].msxinfo.startcluster );
	// if there is no cluster assigned yet to this file, then find a free cluster
	bool followFATClusters=true;
	if (curcl == 0) {
		followFATClusters=false;
		curcl=findFirstFreeCluster();
	setsh(mapdir[dirindex].msxinfo.startcluster, curcl);
	}
	PRT_DEBUG("Starting at cluster " << curcl );

	int size = fsize;
	int prevcl = 0; 
	while (size && (curcl <= MAX_CLUSTER)) {
		clustermap[curcl].dirEntryNr = dirindex;
		clustermap[curcl].fileOffset = fsize - size;

		size -= (size > (SECTOR_SIZE * 2) ? (SECTOR_SIZE * 2) : size);
		if (prevcl) {
			WriteFAT(prevcl, curcl);
		}
		prevcl = curcl;
		//now we check if we continue in the current clusterstring or need to allocate extra unused blocks
		if (followFATClusters){
			curcl=ReadFAT(curcl);
			if ( curcl == EOF_FAT ) {
				followFATClusters=false;
				curcl=findFirstFreeCluster(); 
			}
		} else {
			do {
				curcl++;
			} while((curcl <= MAX_CLUSTER) && ReadFAT(curcl));
		}
		PRT_DEBUG("Continuing at cluster " << curcl);
	}
	if ((size == 0) && (curcl <= MAX_CLUSTER)) {
		WriteFAT(prevcl, EOF_FAT); //probably at an if(prevcl==0)WriteFAT(curcl, EOF_FAT) } else {} if I checked what an MSX does with filesize zero and fat allocation;

		//clear remains of FAT if needed
		if (followFATClusters) {
			while((curcl <= MAX_CLUSTER) && (curcl != EOF_FAT )) {
				prevcl=curcl;
				curcl=ReadFAT(curcl);
				WriteFAT(prevcl, 0);
				clustermap[prevcl].dirEntryNr = NODIRENTRY;
				clustermap[prevcl].fileOffset = 0;
			}
			WriteFAT(prevcl, 0);
			clustermap[prevcl].dirEntryNr = NODIRENTRY;
			clustermap[prevcl].fileOffset = 0;
		}
	} else {
		//TODO: don't we need a EOF_FAT in this case as well ?
		// find out and adjust code here
		PRT_INFO("Fake Diskimage full: " << mapdir[dirindex].filename << " truncated.");
	}
	//write (possibly truncated) file size
	setlg(mapdir[dirindex].msxinfo.size, fsize - size);

}

void FDC_DirAsDSK::write(byte track, byte sector, byte side, 
                    int size, const byte* buf)
{
	assert(size == SECTOR_SIZE);
	
	int logicalSector = physToLog(track, side, sector);
	if (logicalSector >= nbSectors) {
		throw NoSuchSectorException("No such sector");
	}
	PRT_DEBUG("Writing sector : " << logicalSector );
	if (logicalSector == 0) {
		//copy buffer into our fake bootsector and safe into file
		PRT_DEBUG("Reading boot sector");
		memcpy(BootBlock, buf, SECTOR_SIZE);
		std::string filename(MSXrootdir);
		filename+="/"+BootBlockFileName ;
		FILE* file = fopen(filename.c_str(), "wb");
		if (file) {
			PRT_INFO("Writing bootblock to " << filename);
			fwrite(buf, 1, size, file);
			fclose(file);
		} else {
			PRT_INFO("Couldn't create bootsector file" << filename);
		}

	} else {
	  //
	  //   for now simply ignore writes
	  //
	  throw WriteProtectedException(
		  "Writing not yet supported for FDC_DirAsDSK");
	}

}

void FDC_DirAsDSK::readBootSector()
{
	// We can fake regular DS or SS disks
	if (nbSectors == 1440) {
		sectorsPerTrack = 9;
		nbSides = 2;
	} else if (nbSectors == 720) {
		sectorsPerTrack = 9;
		nbSides = 1;
	}
}


bool FDC_DirAsDSK::writeProtected()
{
	return false ; // for the moment we don't allow writing to this directory
}

bool FDC_DirAsDSK::doubleSided()
{
	return nbSides == 2;
}

void FDC_DirAsDSK::updateFileInDSK(const string& fullfilename)
{
	struct stat fst;

	if (stat(fullfilename.c_str(), &fst)) {
		PRT_INFO("Error accessing " << fullfilename);
		return;
	}
	if (!S_ISREG(fst.st_mode)) {
		// we only handle regular files for now
		PRT_INFO("Not a regular file: " << fullfilename);
		return;
	}
	    
	if (!checkFileUsedInDSK(fullfilename)) {
		// add file to fakedisk
		PRT_DEBUG("Going to addFileToDSK");
		addFileToDSK(fullfilename);
	} else {
		//really update file
		checkAlterFileInDisk(fullfilename);
	}
}

void FDC_DirAsDSK::addFileToDSK(const string& fullfilename)
{
	//get emtpy dir entry
	int dirindex = 0;
	while ((dirindex < 112) && !mapdir[dirindex].filename.empty()) {
	     dirindex++;
	}
	PRT_DEBUG("Adding on dirindex " << dirindex);
	if (dirindex == 112) {
		PRT_INFO( "Couldn't add " << fullfilename << ": root dir full");
		return;
	}

	// fill in native file name
	mapdir[dirindex].filename = fullfilename;

	// create correct MSX filename
	string MSXfilename = makeSimpleMSXFileName(fullfilename);
	PRT_DEBUG("Using MSX filename " << MSXfilename );
	if (checkMSXFileExists(MSXfilename)) {
		//TODO: actually should increase vfat abrev if possible!!
		PRT_INFO("Couldn't add " << fullfilename << ": MSX name "
		         << MSXfilename<< "existed already");
		return;
	}
	
	// fill in MSX file name
	memcpy(&(mapdir[dirindex].msxinfo.filename), MSXfilename.c_str(), 11);
	// Here actually call to updateFileInDisk!!!!
	updateFileInDisk(dirindex);
	return;
}


} // namespace openmsx
