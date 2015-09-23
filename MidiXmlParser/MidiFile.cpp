#include "ParseExport.h"

/*
MIDIʵ��
1. �ļ�ͷ����
 4d 54 68 64 // ��MThd��
 00 00 00 06 // ����always 6��������6���ֽڵ�����
 00 01 // 0������; 1����棬ͬ��; 2����棬�첽
 00 02 // ���������Ϊ��MTrk���ĸ���
 00 c0 // ����ʱ���ʽ����һ���ķ�������tick����tick��MIDI�е���Сʱ�䵥λ
2. ȫ�ֹ�
 4d 54 72 6b // ��MTrk����ȫ�ֹ�Ϊ������Ϣ(������Ȩ�ٶȺ�ϵͳ��(Sysx)��)
 00 00 00 3d // ����
 
 00 ff 03 // ��������
 05 // ����
 54 69 74 6c 65 // ��Title��
 
 00 ff 02 // ��Ȩ����
 0a // ����
 43 6f 6d 70 6f 73 65 72 20 3a // ��Composer :��
 ����
 00 ff 01 // �����¼�
 09 // ����
 52 65 6d 61 72 6b 73 20 3a // ��Remarks :��
 
 00 ff 51 // �趨�ٶ�xx xx xx����΢��(us)Ϊ��λ�����ķ�������ʱֵ. ���û��ָ����ȱʡ���ٶ�Ϊ 120��/��
 03 // ����
 07 a1 20 // �ķ�����Ϊ 500,000 us���� 0.5s
 
 00 ff 58 // �ĺű��
 04 // ����
 04 02 18 08 // nn dd cc bb �ĺű�ʾΪ�ĸ����֡�nn��dd������Ӻͷ�ĸ����ĸָ����2��dd�η������磬2����4��3����8��cc����һ���ķ�����Ӧ��ռ���ٸ�MIDIʱ�䵥λ��bb����һ���ķ�������ʱֵ�ȼ��ڶ��ٸ�32�������� ��ˣ������� 6 / 8�ĺ�Ӧ�ñ�ʾΪ FF 58 04 06 03 24 08 �����ǣ� 6 / 8�ĺţ� 8����2�����η�����ˣ�������06 03�����ķ�������32��MIDIʱ������ʮ������24����32�����ķ���������8����ʮ����������
 
 00 ff 59 // �׺���Ϣ
 02 // ����
 00 00 // sf mf ��sfָ���������������š����ŵ���Ŀ�����磬A�������������ע���������ţ���ôsf=03�����磬F�������������д��һ�����ţ���ôsf=81��Ҳ����˵��������Ŀд��0x��������Ŀд��8x ��mfָ�������Ǵ������С�������mf=00��С��mf=01��
 00 ff 2f 00 // ������ֹ
3. ��ͨ���졡��
 4d 54 72 6b // ��MTrk������ͨ����
 00 00 01 17 // ����
 ����
 00 ff 03 // 00: delta_time; ff 03:Ԫ�¼�����������
 06 // ����
 43 20 48 61 72 70 // ��C Harp��
 
 00 b0 00 00 // 00:delta_time; bn:����nͨ��������; xx:���������; xx:������ֵ���˴�Ϊ����0ͨ��0�ſ�����ֵΪ0��
 00 b0 20 00 // �˴�Ϊ����0ͨ��32�ſ�����ֵΪ0��
 00 c0 16    // 00:delta_time; cn:����nͨ����ɫ; xx:��ɫֵ���˴�Ϊ����0ͨ����ɫֵΪ22 Accordion �ַ��١�
 84 40 b0 65 00 // 84 40:delta_time; �˴�Ϊ����0ͨ��101�ſ�����ֵΪ0��
 00 b0 64 00 // �˴�Ϊ����0ͨ��100�ſ�����ֵΪ0��
 00 b0 06 18 // �˴�Ϊ����0ͨ��6�ſ�����ֵΪ0��
 00 b0 07 7e // �˴�Ϊ����0ͨ��7�ſ�����(��������)ֵΪ126��
 00 e0 00 40 // 00:delta_time; en:����nͨ������; xx yy:��ȡ��7bit���14bitֵ���˴�Ϊ����0ͨ������ֵΪ64��
 00 b0 0a 40 // �˴�Ϊ����0ͨ��7�ſ�����(��������)ֵΪ126��
 
 00 90 43 40 // 00:delta_time; 9n:��nͨ������; xx yy: ��һ���������������š���128��������MIDI�豸�����Ϊ0��127������������C����������60���� �ڶ��������ֽ����ٶȣ���0��127��һ��ֵ����������ö����������ࡣ һ���ٶ�Ϊ��Ŀ�ʼ������Ϣ����Ϊ����ʵ�ϵ�һ��ֹͣ��������Ϣ���˴�Ϊ��64���ȷ���67������
 81 10 80 43 40 // 81 10:delta_time; 8n:�ر�nͨ������; xx yy: ��һ���������������š���128��������MIDI�豸�����Ϊ0��127������������C����������60���� �ڶ��������ֽ����ٶȣ���0��127��һ��ֵ����������ö����������ࡣ һ���ٶ�Ϊ��Ŀ�ʼ������Ϣ����Ϊ����ʵ�ϵ�һ��ֹͣ��������Ϣ���˴�Ϊ��64���ȹر�67������
 00 90 43 40
 30 80 43 40
 00 90 45 40
 81 40 80 45 40
 00 90 43 40
 81 40 80 43 40
 00 90 48 40
 81 40 80 48 40
 00 90 47 40
 ����83 00 80 47 40
 ����00 90 43 40
 ����81 10 80 43 40
 ����00 90 43 40
 ����30 80 43 40
 ����00 90 45 40
 ����81 40 80 45 40
 ����00 90 43 40
 ����81 40 80 43 40
 ����00 90 4a 40
 ����81 40 80 4a 40
 ����00 90 48 40
 ����83 00 80 48 40
 ����00 90 43 40
 ����81 10 80 43 40
 ����00 90 43 40
 ����30 80 43 40
 ����00 90 4f 40
 ����81 40 80 4f 40
 ����00 90 4c 40
 ����81 40 80 4c 40
 ����00 90 48 40
 ����81 40 80 48 40
 ����00 90 47 40
 ����81 40 80 47 40
 ����00 90 45 40
 ����83 00 80 45 40
 ����00 90 4d 40
 ����81 10 80 4d 40
 ����00 90 4d 40
 ����30 80 4d 40
 ����00 90 4c 40
 ����81 40 80 4c 40
 ����00 90 48 40
 ����81 40 80 48 40
 ����00 90 4a 40
 ����81 40 80 4a 40
 ����00 90 48 40
 ����83 00 80 48 40
 ����01 b0 7b 00 // 00:delta_time; bn:����nͨ��������; xx:���������; xx:������ֵ���˴�Ϊ����0ͨ��123�ſ�����(�ر���������)ֵΪ0��
 ����00 b0 78 00 // 00:delta_time; bn:����nͨ��������; xx:���������; xx:������ֵ���˴�Ϊ����0ͨ��120�ſ�����(�ر���������)ֵΪ0��
 00 ff 2f 00 // ������ֹ

 �±��г��������������Ӧ�������ǡ�
 �˶�����||                    ������
 #  ||
     || C   | C#  |  D  | D#  |  E  |  F  | F#  |  G  | G#  |  A  | A#  | B
 -----------------------------------------------------------------------------
 0  ||  12 |  13 |  14 |  15 |  16 |  17 |  18 |  19 |  20 |  21 |  22 | 23
 1  ||  24 |  25 |  26 |  27 |  28 |  29 |  30 |  31 |  32 |  33 |  34 | 35
 2  ||  36 |  37 |  38 |  39 |  40 |  41 |  42 |  43 |  44 |  45 |  46 | 47
 3  ||  48 |  49 |  50 |  51 |  52 |  53 |  54 |  55 |  56 |  57 |  58 | 59
 4  ||  60 |  61 |  62 |  63 |  64 |  65 |  66 |  67 |  68 |  69 |  70 | 71
 5  ||  72 |  73 |  74 |  75 |  76 |  77 |  78 |  79 |  80 |  81 |  82 | 83
 6  ||  84 |  85 |  86 |  87 |  88 |  89 |  90 |  91 |  92 |  93 |  94 | 95
 7  ||  96 |  97 |  98 |  99 | 100 | 101 | 102 | 103 | 104 | 105 | 106 | 107
 8  || 108 | 109 | 110 | 111 | 112 | 113 | 114 | 115 | 116 | 117 | 118 | 119
 9  || 120 | 121 | 122 | 123 | 124 | 125 | 126 | 127 |
 
 �˶�����||                    ������
 #  ||
    || C   | C#  |  D  | D#  |  E  |  F  | F#  |  G  | G#  |  A  | A#  | B
 -----------------------------------------------------------------------------
 0  ||   C |   D |   E |   F |  10 |  11 |  12 |  13 |  14 |  15 |  16 | 17
 1  ||  18 |  19 |  1A |  1B |  1C |  1D |  1E |  1F |  20 |  21 |  22 | 23
 2  ||  24 |  25 |  26 |  27 |  28 |  29 |  2A |  2B |  2C |  2D |  2E | 2F
 3  ||  30 |  31 |  32 |  33 |  34 |  35 |  36 |  37 |  38 |  39 |  3A | 3B
 4  ||  3C |  3D |  3E |  3F |  40 |  41 |  42 |  43 |  44 |  45 |  46 | 47
 5  ||  48 |  49 |  4A |  4B |  4C |  4D |  4E |  4F |  50 |  51 |  52 | 53
 6  ||  84 |  85 |  86 |  87 |  88 |  89 |  90 |  91 |  92 |  93 |  94 | 95
 7  ||  96 |  97 |  98 |  99 | 100 | 101 | 102 | 103 | 104 | 105 | 106 | 107
 8  || 108 | 109 | 110 | 111 | 112 | 113 | 114 | 115 | 116 | 117 | 118 | 119
 9  || 120 | 121 | 122 | 123 | 124 | 125 | 126 | 127 |

 tt 9n xx vv //tt: delta_time; 9n: ��nͨ������; xx: ����00~7F; vv:����00~7F
 tt 8n xx vv //tt: delta_time; 8n: �ر�nͨ������; xx: ����00~7F; vv:����00~7F
 
 case 0xa0: //���������Ժ�  ����:00~7F ����:00~7F
 case 0xb0: //������  ����������:00~7F ����������:00~7F
 case 0xc0: //�л���ɫ�� ��������:00~7F
 case 0xd0: //ͨ������ѹ�����ɽ�����Ϊ�������� ֵ:00~7F
 case 0xe0: //���� ����(Pitch)��λ:Pitch mod 128  ���߸�λ:Pitch div 128
 */

ITrack* MidiFile::getTrackPianoTrack()
{
	ITrack* track0 = NULL;

	//search the piano track
	for (auto track = tracks.begin(); track != tracks.end(); track++) {
		if (track->events.size() > 0) {
			track0 = &(*track);
			bool foundPiano = false;
			for (auto event = track->events.begin(); event != track->events.end(); event++) {
				//c9 �Ǵ����channel
				if (event->evt != 0xC9 && (event->evt & 0xF0) == 0xC0 && event->nn == 0) {
					foundPiano = true;
					break;
				}
			}
			if (foundPiano)
				break;
		}
	}
	return track0;
}

double MidiFile::secPerTick()
{
	int ticksPerQuarter = quarter;
	int usPerQuarter = tempos.front().tempo;
	return usPerQuarter/1000000.0/ticksPerQuarter;
}

bool MidiFile::sort_ascending_order_tick(const Event& obj1, const Event& obj2)
{
	if (obj1.tick < obj2.tick) {
		return true;
	} else if (obj1.tick > obj2.tick) {
		return false;
	} else {
		if (obj1.play_priority > obj2.play_priority) {
			return true;
		} else if (obj1.play_priority < obj2.play_priority) {
			return false;
		} else {
			if (obj1.track_priority < obj2.track_priority) {
				return true;
			} else if (obj1.track_priority > obj2.track_priority) {
				return false;
			} else {
				return obj1.nn < obj2.nn;
			}
		}
	}
}

std::vector<Event>& MidiFile::mergedMidiEvents()
{
	if (_mergedMidiEvents.empty())
	{
		int tracksHaveEvents = 0;
		for (int i = 0; i < tracks.size(); ++i)
		{
			ITrack& track = tracks[i];
			if (track.events.size() > 0)
				tracksHaveEvents++;
		}

		if (tracksHaveEvents <= 1) {
			onlyOneTrack = true;
		} else if (tracks.size() > maxTracks) {
			int top1 = 0, top1_num = 0;
			int top2 = 1, top2_num = 0;
			for (int i = 0; i < tracks.size(); i++)
			{
				if (tracks[i].events.size() > top1_num) {
					top2 = top1;
					top2_num = top1_num;
					top1 = i;
					top1_num = tracks[i].events.size();
				} else if (tracks[i].events.size() > top2_num) {
					top2 = i;
					top2_num = tracks[i].events.size();
				}
			}
			if (top1 != top2)
			{
				ITrack* track0, *track1;
				if (top1 > top2) {
					track0 = &tracks[top2];
					track1 = &tracks[top1];
				} else {
					track0 = &tracks[top1];
					track1 = &tracks[top2];
				}

				for (auto e = track0->events.begin(); e != track0->events.end(); e++)
					e->track = 0;
				for (auto e = track1->events.begin(); e != track1->events.end(); e++)
					e->track = 1;
			}
		}

		int track_index = 0;
		for (auto track = tracks.begin(); track != tracks.end(); track++, track_index++) {
			if (!track->events.empty()) {
				//_mergedMidiEvents.insert(_mergedMidiEvents.end(), track->events.begin(), track->events.end());
				for (auto midi_event = track->events.begin(); midi_event != track->events.end(); midi_event++) {
					midi_event->track_priority = track_index;
					if (0x80 == (midi_event->evt & 0xF0) || (0x90 == (midi_event->evt & 0xF0) && 0 == midi_event->vv))
						midi_event->play_priority = 1;		//the note coresponding with this event off, means its priority is higher
					_mergedMidiEvents.push_back(*midi_event);
				}
			}
		}
		std::sort(_mergedMidiEvents.begin(), _mergedMidiEvents.end(), sort_ascending_order_tick);

		//remove the notes with same tick, evt, nn
		std::set<int> duplicateEvents;
		for (int i = 0; i < _mergedMidiEvents.size(); i++)
		{
			auto& event = _mergedMidiEvents[i];
			int evt = event.evt & 0xF0;
			int channel = event.evt & 0x0F;
			if (0x90 == evt && event.vv > 0) {		//note on 
				for (int k = i+1; k < _mergedMidiEvents.size(); k++) {
					auto& nextEvent = _mergedMidiEvents[k];
					int next_evt = nextEvent.evt & 0xF0;
					int next_channel = nextEvent.evt & 0x0F;
					if (nextEvent.tick != event.tick) {
						break;
					} else {
						if (nextEvent.nn == event.nn && (0x90 == next_evt && next_channel == channel && nextEvent.vv > 0))
						{
							duplicateEvents.insert(k);
							for (int off = k+1; off < _mergedMidiEvents.size(); off++)
							{
								auto& offEvent = _mergedMidiEvents[off];
								int off_evt = offEvent.evt & 0xF0;
								int off_channel = offEvent.evt & 0x0F;
								if ((0x80 == off_evt || (0x90 == off_evt && 0 == offEvent.vv)) && channel == off_channel && offEvent.nn == nextEvent.nn)
								{
									duplicateEvents.insert(off);
									break;
								}
							}
						}
					}
				}
			}
		}
		if (duplicateEvents.size() > 0)
		{
			printf("tracks:%d, remove %d duplicate events\n", tracks.size(), duplicateEvents.size());
			for (auto rit = duplicateEvents.rbegin(); rit != duplicateEvents.rend(); rit++)
			{
				int index = 0;
				for (auto event = _mergedMidiEvents.begin(); event != _mergedMidiEvents.end(); event++, index++) {
					if (index == *rit) {
						_mergedMidiEvents.erase(event);
						break;
					}
				}
			}
		}

		//����Ƿ����ص���event��ͬһ������ǰ�滹û�н�����������ֿ�ʼ��
		int changedEvent = 0;
		for (int i = 0; i < _mergedMidiEvents.size(); i++)
		{
			auto& event = _mergedMidiEvents[i];
			if (0x90 == (event.evt & 0xF0) && event.vv > 0)		//note on
			{
				int nextStartTick = -1;
				for (int j = i+1; j < _mergedMidiEvents.size(); j++)
				{
					auto& nextEvent = _mergedMidiEvents[j];
					if (nextEvent.nn == event.nn && (event.evt & 0x0F) == (nextEvent.evt & 0x0F)) {
						if (0x80 == (nextEvent.evt & 0xF0) || (0x90 == (nextEvent.evt & 0xF0) && 0 == nextEvent.vv)) {		//note off
							if (nextStartTick > 0 && nextEvent.tick > event.tick+480*8)
							{
								nextEvent.tick = nextStartTick;
								changedEvent++;
							}
							break;
						} else if (0x90 == (nextEvent.evt & 0xF0) && nextEvent.vv > 0) {		//note on
							nextStartTick = nextEvent.tick;
						}
					}
				}
			}
		}
		if (changedEvent > 0)
			std::sort(_mergedMidiEvents.begin(), _mergedMidiEvents.end(), sort_ascending_order_tick);
	}
	return _mergedMidiEvents;
}