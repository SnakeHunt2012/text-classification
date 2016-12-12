#include <sstream>
#include "NewsSegment.h"

#include "strnormalize.h"

// for segment
#include "segmenter.h"
#include "config.h"
#include "transcode.h"

int NewsSegment_init(void **h, const char *conf)
{
	qss::segmenter::Config::get_instance()->init(conf);
	qss::segmenter::Segmenter* m_segmenter = qss::segmenter::CreateSegmenter();
	if(m_segmenter == NULL)
	{
		return 1;
	}
	*h = m_segmenter;
	return 0;
}

int NewsSegment_call(void **h, const string& _line, bool pos, string& line_result)
{
	qss::segmenter::Segmenter* m_segmenter = (qss::segmenter::Segmenter*) (*h);	
	int segmode = qss::segmenter::seg_mode::none;
	if(pos) 
	{
		segmode |= qss::segmenter::seg_mode::pos_long;
	}

	string line = _line;

	strnormalize_utf8(line);

	uint16_t *wbuf = new uint16_t[line.length()+100];
	uint16_t *tbuf = new uint16_t[(line.length()+100)*5];
	char *obuf = new char[(line.length()+100)*5];

	try
	{

		//int len = convToucs2(line.c_str(), line.length(), wbuf, line.length()-1, qsrch_code_utf8);
		int len = convToucs2(line.c_str(), line.length(), wbuf, line.length()+100-1, qsrch_code_utf8);
		
		wbuf[len] = 0;
		int segstat = 0;
		len = m_segmenter->segmentUnicode(wbuf, len, tbuf, (line.length()+100)*5, segmode, &segstat);

		//int l = convFromucs2(tbuf, len, obuf, line.length()*5-1, qsrch_code_utf8);
		int l = convFromucs2(tbuf, len, obuf, (line.length()+100)*5-1, qsrch_code_utf8);
		
		obuf[l] = '\0';
	}
	catch (exception& e)
	{
		delete[] obuf;
		delete[] wbuf;
		delete[] tbuf;
		return 1;
	}

	ostringstream os;
	os << obuf;
	line_result = os.str();

	delete[] obuf;
	delete[] wbuf;
	delete[] tbuf;

	return 0;
}
