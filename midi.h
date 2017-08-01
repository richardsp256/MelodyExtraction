struct Track{
  unsigned char* data;
  int len;
};

struct Midi{
  struct Track** tracks;
  int format; //0 = single track, 1 = multitrack, 2 = multisong.
  int numTracks; //number of tracks
  int division; //delts time. if positive, represents units per beat. if negative, in SMPTE compatible units. 
};
struct Track* GenerateTrack(int* noteArr, int size, int verbose);
struct Midi* GenerateMIDI(int* noteArr, int size, int verbose);
void freeMidi(struct Midi* midi);
int isMidiNote(int note);
void NoteToName(int n, char** name);
int FrequencyToNote(double freq);
float FrequencyToFractionalNote(double freq);
double log2(double x);
void SaveMIDI(struct Midi* midi, char* path, int verbose);
void AddHeader(FILE** f, short format, short tracks, short division);
void AddTrack(FILE** f, unsigned char* track, int len);
int MakeTrack(unsigned char** track, int trackCapacity, int* noteArr, int size);
void BigEndianInteger(unsigned char** c, int num);
void BigEndianShort(unsigned char** c, short num);
int IntToVLQ(unsigned int num, unsigned char** VLQ);
unsigned char* MessageNoteOn(int pitch, int velocity);
unsigned char* MessageNoteOff(int pitch, int velocity);