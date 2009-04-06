// ==========================================================================
//code=EUC	tab=4
//
// wizd:	MediaWiz Server daemon.
//
// 		wizd_tools.c
//											$Revision: 1.22 $
//											$Date: 2005/01/26 13:32:14 $
//
//	���٤Ƽ�����Ǥ�Ǥ��ʤ������ޤġ�
//  ���Υ��եȤˤĤ���VertexLink���䤤��碌�ʤ��Ǥ���������
// ==========================================================================
#include        <sys/types.h>

#include 		<ctype.h>
#include        <stdio.h>
#include		<stdlib.h>
#include 		<stdarg.h>
#include		<string.h>
#include		<limits.h>
#include 		<time.h>
#include 		<sys/stat.h>
#include        <errno.h>
#include 		<sys/types.h>
#include 		<fcntl.h>
#include 		<unistd.h>

#include		"wizd.h"
#include		"wizd_tools.h"


static unsigned char		debug_log_filename[WIZD_FILENAME_MAX];	// �ǥХå������ϥե�����̾(�ե�ѥ�)
static unsigned char		debug_log_initialize_flag  = (-1);	// �ǥХå���������ե饰


/********************************************************************************/
// sentenceʸ�������keyʸ�����repʸ������ִ����롣
/********************************************************************************/
void replase_character(unsigned char *sentence, int sentence_buf_size, const unsigned char *key, const unsigned char *rep)
{
	int	malloc_size;
	unsigned char	*p, *buf;


	if ( strlen(key) == 0 )
		return;

	malloc_size = strlen(sentence) * 4;
	buf = malloc(malloc_size);
	if ( buf == NULL )
		return;

	p = strstr(sentence, key);

	while (p != NULL)
	{
		*p = '\0';
		strncpy(buf, p+strlen(key), malloc_size );
		strncat(sentence, rep, sentence_buf_size - strlen(sentence) );
		strncat(sentence, buf, sentence_buf_size - strlen(sentence) );
		p = strstr(p+strlen(rep), key);
	}

	free(buf);

	return;
}

/********************************************************************************/
// sentenceʸ������κǽ��keyʸ�����repʸ������ִ����롣
/********************************************************************************/
void replase_character_first(unsigned char *sentence, int sentence_buf_size, const unsigned char *key, const unsigned char *rep)
{
	int	malloc_size;
	unsigned char	*p, *buf;


	if ( strlen(key) == 0 )
		return;

	malloc_size = strlen(sentence) * 4;
	buf = malloc(malloc_size);
	if ( buf == NULL )
		return;

	p = strstr(sentence, key);

	if (p != NULL)
	{
		*p = '\0';
		strncpy(buf, p+strlen(key), malloc_size );
		strncat(sentence, rep, sentence_buf_size - strlen(sentence) );
		strncat(sentence, buf, sentence_buf_size - strlen(sentence) );
	}

	free(buf);

	return;
}

// **************************************************************************
// sentence ʸ������Ρ�start_keyʸ�����end_keyʸ����˶��ޤ줿��ʬ��������
// **************************************************************************
void cut_enclose_words(unsigned char *sentence, int sentence_buf_size, unsigned char *start_key, unsigned char *end_key)
{
	int	malloc_size;
	unsigned char *start_p, *end_p, *buf;

	malloc_size = strlen(sentence) *2;
	buf = malloc(malloc_size);
	if ( buf == NULL )
		return;

	// start_key��õ����
	start_p = strstr(sentence, start_key);

	while ( start_p != NULL )
	{
		// start_key ����
		*start_p = '\0';

		// start_p �θ���buf�˥��ԡ�
		strncpy(buf, start_p+strlen(start_key), malloc_size );

		// end_key��õ��
		end_p = strstr(buf, end_key);

		if ( end_p == NULL )
		{
			// end_key��̵����С�

			// �������start_key ������
			strncat( sentence, start_key, sentence_buf_size - strlen(sentence) );

			//����������äĤ��롣
			strncat( sentence, buf, sentence_buf_size - strlen(sentence) );
			break;
		}

		// end_key ���Ĥ��ä���

		// end_key�θ���sentence�ˤ��äĤ��롣
		strncat( sentence, end_p+strlen(end_key), sentence_buf_size - strlen(sentence) );

		// ����start_key��õ����
		start_p = strstr(sentence, start_key);
	}


	free(buf);
	return;
}


//***************************************************************************
// sentenceʸ�����ꡢcut_char���������
//	���Ĥ���ʤ���в��⤷�ʤ���
//***************************************************************************
void 	cut_after_character(unsigned char *sentence, unsigned char cut_char)
{
	unsigned char 	*symbol_p;

	// ����оݥ���饯���������ä���硢���줫���������
	symbol_p = strchr(sentence, cut_char);

	if (symbol_p != NULL)
	{
		*symbol_p = '\0';
	}

	return;
}



//***************************************************************************
// sentenceʸ����Ρ�cut_char���ǽ�˽ФƤ����꤫��������
// �⤷��cut_char��sentenceʸ��������äƤ��ʤ��ä���硢ʸ�����������
//***************************************************************************
void 	cut_before_character(unsigned char *sentence, unsigned char cut_char)
{

	unsigned char 	*symbol_p;
	unsigned char	*malloc_p;
	int				sentence_len;


	if (sentence == NULL)
		return;

	sentence_len = strlen(sentence);


	// ����оݥ���饯�������ǽ�˽ФƤ�����õ����
	symbol_p = strchr(sentence, cut_char);
	if (symbol_p == NULL)
	{
		// ȯ���Ǥ��ʤ��ä���硢ʸ�������������
		strncpy(sentence, "", sentence_len);
		return;
	}

	symbol_p++;

	// �ƥ�ݥ�ꥨ�ꥢmalloc.
	malloc_p = malloc(sentence_len + 10);
	if (malloc_p == NULL)
		return;

	// ����оݥ���饯�����θ����Ǹ�ޤǤ�ʸ����򥳥ԡ�
	strncpy(malloc_p, symbol_p, sentence_len + 10);

	// sentence�񤭴���
	strncpy(sentence, malloc_p, sentence_len);

	free(malloc_p);

	return;
}


//************************************************************************
// sentenceʸ����Ρ�cut_char���Ǹ�˽ФƤ����꤫��������
// �⤷��cut_char��sentenceʸ��������äƤ��ʤ��ä���硢�ʤˤ⤷�ʤ���
//************************************************************************
void 	cut_before_last_character(unsigned char *sentence, unsigned char cut_char)
{

	unsigned char 	*symbol_p;
	unsigned char	*malloc_p;
	int				sentence_len;

	if (sentence == NULL)
		return;

	sentence_len = strlen(sentence);

	// ����оݥ���饯�������Ǹ�˽ФƤ�����õ����
	symbol_p = strrchr(sentence, cut_char);
	if (symbol_p == NULL)
	{
		// ȯ���Ǥ��ʤ��ä���硢�ʤˤ⤷�ʤ���
		return;
	}

	symbol_p++;

	// �ƥ�ݥ�ꥨ�ꥢmalloc.
	malloc_p = malloc(sentence_len + 10);
	if (malloc_p == NULL)
		return;

	// ����оݥ���饯�����θ����Ǹ�ޤǤ�ʸ����򥳥ԡ�
	strncpy(malloc_p, symbol_p, sentence_len + 10);

	// sentence�񤭴���
	strncpy(sentence, malloc_p, sentence_len);

	free(malloc_p);

	return;
}

//************************************************************************
// sentenceʸ����Ρ�cut_char���Ǹ�˽ФƤ����꤫�����CUT
// �⤷��cut_char��sentenceʸ��������äƤ��ʤ��ä���硢ʸ�������������
//************************************************************************
void 	cut_after_last_character(unsigned char *sentence, unsigned char cut_char)
{

	unsigned char 	*symbol_p;
	int				sentence_len;

	if (sentence == NULL)
		return;

	sentence_len = strlen(sentence);

	// ����оݥ���饯�������Ǹ�˽ФƤ�����õ����
	symbol_p = strrchr(sentence, cut_char);
	if (symbol_p == NULL)
	{
		// ȯ���Ǥ��ʤ��ä���硢ʸ�������������
		strncpy(sentence, "", sentence_len);
		return;
	}

	*symbol_p = '\0';

	return;
}


//******************************************************************
// sentence�Ρ���� n byte��Ĥ��ƺ����
//******************************************************************
void 	cat_before_n_length(unsigned char *sentence,  unsigned int n)
{
	unsigned char	*malloc_p;
	unsigned char	*work_p;

	int				sentence_len;

	if (sentence == NULL)
		return;

	sentence_len = strlen(sentence);

	// sentence ����n����Ʊ����û���ʤ��return
	if ( sentence_len <= n )
		return;


	// �ƥ�ݥ�ꥨ�ꥢmalloc.
	malloc_p = malloc(sentence_len + 10);
	if (malloc_p == NULL)
		return;

	work_p = sentence;
	work_p += sentence_len;
	work_p -= n;
	strncpy(malloc_p, work_p, sentence_len + 10);
	strncpy( sentence, malloc_p, sentence_len);

	free(malloc_p);
	return;
}

//******************************************************************
// sentence�Ρ���� n byte����
//  ��Ĺ��n byte�������ʤ��ä��顢ʸ�����������
//******************************************************************
void 	cat_after_n_length(unsigned char *sentence,  unsigned int n)
{
	unsigned char	*work_p;
	int				sentence_len;

	if (sentence == NULL)
		return;

	sentence_len = strlen(sentence);

	// sentence ����n����Ʊ����û���ʤ�С������������return;
	if ( sentence_len <= n )
	{
		strncpy(sentence, "", sentence_len);
		return;
	}


	// ��� n byte����
	work_p = sentence;
	work_p += sentence_len;
	work_p -= n;
	*work_p = '\0';

	return;
}



//******************************************************************
// sentenceʸ����Ρ�cut_char��ȴ����
//******************************************************************
void 	cut_character(unsigned char *sentence, unsigned char cut_char)
{

	unsigned char 	*symbol_p;
	unsigned char	*malloc_p;
	unsigned char	*work_p;
	int				sentence_len;


	if (sentence == NULL)
		return;

	sentence_len = strlen(sentence);

	// �ƥ�ݥ�ꥨ�ꥢmalloc.
	malloc_p = malloc(sentence_len + 10);
	if (malloc_p == NULL)
		return;

	symbol_p = sentence;
	work_p = malloc_p;


	// �����롼�ס�
	while (*symbol_p != '\0')
	{
		// ����оݤΥ���饯�����������顢��������Ф���
		if (*symbol_p == cut_char)
		{
			symbol_p++;
		}
		else	// ����оݥ���饯�����ճ����ä��顢���ԡ���
		{
			*work_p = *symbol_p;
			work_p++;
			symbol_p++;
		}
	}

	// '\0' �򥳥ԡ���
	*work_p = *symbol_p;

	// sentence�񤭴���
	strncpy(sentence, malloc_p, sentence_len);

	free(malloc_p);

	return;
}

//******************************************************************
// sentenceʸ����Ρ�Ƭ��cut_char�������顢ȴ����
//******************************************************************
void 	cut_first_character(unsigned char *sentence, unsigned char cut_char)
{

	unsigned char	*malloc_p;
	unsigned char	*work_p;
	int				sentence_len;

	if (sentence == NULL)
		return;

	sentence_len = strlen(sentence);

	// �ƥ�ݥ�ꥨ�ꥢmalloc.
	malloc_p = malloc(sentence_len + 10);
	if (malloc_p == NULL)
		return;

	strncpy(malloc_p, sentence, sentence_len + 10);

	work_p = malloc_p;



	// ����оݥ���饯���������뤫����ʤ�롣
	while ((*work_p == cut_char) && (*work_p != '\0'))
	{
		work_p++;
	}

	// sentence�񤭴���
	strncpy(sentence, work_p, sentence_len);

	free(malloc_p);

	return;
}

// ***************************************************************************
// sentenceʸ����ι����ˡ�cut_char�����ä��Ȥ������
// ***************************************************************************
void 	cut_character_at_linetail(char *sentence, char cut_char)
{
	char	*source_p;
	int		length, i;


	if (sentence == NULL)
		return;

	length = strlen(sentence);	// ʸ����ĹGet

	source_p = sentence;
	source_p += length;		// ����ݥ��󥿤�ʸ����κǸ�˥��åȡ�


	for (i=0; i<length; i++)	// ʸ����ο����������֤���
	{
		source_p--;			// ��ʸ���������ء�

		if (*source_p == cut_char)	// �������� �ҥåȤ��������
		{
			*source_p = '\0';
		}
		else						// �㤦����餬�ФƤ����Ȥ���ǽ�λ��
		{
			break;
		}
	}

	return;
}



/********************************************************************************/
// sentenceʸ�������unique_char��Ϣ³���Ƥ���Ȥ����unique_char1ʸ�������ˤ��롣
/********************************************************************************/
void duplex_character_to_unique(unsigned char *sentence, unsigned char unique_char)
{
	unsigned char	*source_p, *work_p;
	unsigned char 	*work_malloc_p;
	unsigned char	unique_char_count = 0;
	int				org_sentence_len;


	if (sentence == NULL)
		return;

	// ���ꥸ�ʥ�ʸ����Ĺ����¸��
	org_sentence_len = strlen(sentence);

	// ����Хåե����ݡ�
	work_malloc_p = malloc( org_sentence_len+10 );
	if ( work_malloc_p == NULL )
		return;

	source_p = sentence;
	work_p = work_malloc_p;

	// sensenseʸ���󤫤顢unique_char�ʳ������إ��ԡ���
	while (*source_p != '\0')
	{
		if (*source_p == unique_char)	// unique_charȯ��
		{
			if (unique_char_count == 0)	// �ǽ�ΰ�Ĥʤ饳�ԡ�������ʳ��ʤ饹���åס�
			{
				*work_p = *source_p;
				work_p++;
			}
			unique_char_count++;
		}
		else	// unique_char �ʳ��ʤ饳�ԡ���
		{
			unique_char_count = 0;
			*work_p = *source_p;
			work_p++;
		}
		source_p++;
	}

	*work_p = '\0';

	// ������顢sentence�ء���̤���᤹��
	strncpy(sentence, work_malloc_p, org_sentence_len );

	free( work_malloc_p );	// Mem Free.

	return;
}




//*********************************************************
// sentenceʸ�����ꡢ�ǽ�˽Ф��褿cut_char�������ʬ�䡣
//
//	sentence	(IN) ʬ���оݤ�ʸ�����Ϳ���롣
//	cut_char	(IN) ʬ���оݤ�ʸ��������롣
//	split1		(OUT)���åȤ��줿������ʬ�����롣sentence��Ʊ���Υ�������˾�ޤ�����
//	split2		(OUT)���åȤ��줿������ʬ�����롣sentence��Ʊ���Υ�������˾�ޤ�����
//
//
// return
//		0: 			���ｪλ��
//		����ʳ���	���顼��ʬ�伺�Ԥʤɤʤɡ�
//*********************************************************
int sentence_split(unsigned char *sentence, unsigned char cut_char, unsigned char *split1, unsigned char *split2)
{
	unsigned char 	*p;
	unsigned char	*malloc_p;
	int				sentence_len;

	// ���顼�����å���
	if ((sentence == NULL)||(split1 == NULL)||(split2 == NULL))
	{
		return 1;	// ������NULL�ޤ��ꡣ
	}

	// sentence ��Ĺ����Get.
	sentence_len = strlen(sentence);


	// ����ΰ�malloc.
	malloc_p = malloc(sentence_len + 10);

	if (malloc_p == NULL)
	{
		// malloc ���ԡ����顼��
		return 1;
	}

	// sentenceʸ��������˥��ԡ���
	strncpy(malloc_p, sentence, sentence_len + 10);

	// sentence ��ˡ�cut_char ��ͭ�뤫�����å���̵����Х��顼��
	p = strchr(malloc_p, cut_char);
	if (p == NULL)
	{
		free(malloc_p);
		return 1;	// ʬ��ʸ��ȯ���Ǥ�����
	}

	// cut_char��ꡢ���򥫥åȡ�
	*p = '\0';

	// ��Ⱦ��ʬ�򥳥ԡ���
	strncpy(split1, malloc_p, sentence_len);

	// ��Ⱦ��ʬ�򥳥ԡ���
	p++;
	strncpy(split2, p, sentence_len);


	free(malloc_p);

	return 0; // ���ｪλ��
}

//******************************************************************
// filename���顢��ĥ�Ҥ���Ф�('.'��ä���
// '.'��¸�ߤ��ʤ��ä���硢��ĥ�Ҥ�Ĺ���������ϡ�""�����롣
//******************************************************************
void filename_to_extension(unsigned char *filename, unsigned char *extension_buf, int extension_buf_size)
{
	unsigned char	*p;


	// ��ĥ�Ҥ�¸�ߥ����å���
	p = strrchr(filename, '.' );
	if (( p == NULL ) || ( strlen(p) > extension_buf_size ))
	{
		strncpy(extension_buf, "", extension_buf_size );
		return;
	}


	// ��ĥ�Ҥ��ڤ�Ф���
	p++;
	strncpy( extension_buf, p, extension_buf_size );

	return;
}


// **************************************************************************
// text_buf ���顢CR/LF ��'\0'�������ޤǤ��ڤ�Ф��ơ�line_buf��copy��
// (CR/LF��copy����ʤ�)
// ���ιԤ�Ƭ�Υݥ��󥿤�return��
// Error��'\0'�����줿��NULL����롣
// **************************************************************************
unsigned char *buffer_distill_line(unsigned char *text_buf_p, unsigned char *line_buf_p, unsigned int line_buf_size )
{
	unsigned char	*p;
	int			counter = 0;

	p = text_buf_p;

	// ------------------
	// CR/LF '\0'��õ��
	// ------------------
	while ( 1 )
	{
		if ( *p == '\r' ) // CR
		{
			p++;
			continue;
		}

		if ( *p == '\n' )	// LF
		{
			p++;
			break;
		}

		if ( *p == '\0' )
		{
			break;
		}

		p++;
		counter++;
	}

	// --------------------------------------------------
	// ������ʸ����������line_buf_p ��ʸ����򥳥ԡ�
	// --------------------------------------------------
	memset(line_buf_p , '\0', line_buf_size );
	if ( counter >= line_buf_size )
	{
		counter = (line_buf_size -1);
	}
	strncpy(line_buf_p, text_buf_p, counter);

	if ( *p == '\0' )
		return NULL;		// �Хåե��κǸ�
	else
		return p;			// �Хåե�������

}



// **************************************************************************
//  URI���󥳡��ɤ�Ԥ��ޤ�.
//  ��ǽ : URI�ǥ����ɤ�Ԥ�
//  �� : int uri_encode
//  (char* dst,size_t dst_len,const char* src,int src_len);
//  ���� : dst �Ѵ�����ʸ���ν񤭽Ф���.
// 		   dst_len �Ѵ�����ʸ���ν񤭽Ф���κ���Ĺ.
// 		   src �Ѵ�����ʸ��.
// 		   src_len �Ѵ�����ʸ����Ĺ��.
//  ���� : ���󥳡��ɤ���ʸ���ο�(���Τޤޤ�ޤ�)
// **************************************************************************
int uri_encode(unsigned char *dst,  unsigned int dst_len, const unsigned char *src, int src_len)
{
	int idx_src;
	int idx_dst;
	int cnt;


	// ���������å�
	if((dst == NULL) || (dst_len < 1))
	{
		return 0;
	}

	if((src == NULL) || (src_len < 1)) 
	{
		*dst = 0;
		return 0;
	}
	cnt = 0;
	for (idx_src = idx_dst = 0 ; (idx_src < src_len) && (idx_dst < dst_len) && (src[idx_src] != '\0'); idx_src++)
	{
		/* ���󥳡��ɤ��ʤ�ʸ������ */
		if ( strchr("!$()*,-./:;@[\\]^_`{}~", src[idx_src]) != NULL )
		{
			dst[idx_dst] = src[idx_src];
			idx_dst += 1;
		}
		/* ����ե��٥åȤȿ����ϥ��󥳡��ɤ������Τޤ� */
		else if ( isalnum( src[idx_src] ) )
		{
			dst[idx_dst] = src[idx_src];
			idx_dst += 1;
		}
		/* ����ʳ��Ϥ��٤ƥ��󥳡��� */
		else
		{
			if ((idx_dst + 3) > dst_len)
				break;

			idx_dst += sprintf(&dst[idx_dst],"%%%02X",(unsigned char)(src[idx_src]));
		}
		cnt++;

		if ((idx_dst + 1) < dst_len)
		{
			dst[idx_dst] = '\0';
		}
	}

	return cnt;
}


// **************************************************************************
// URI�ǥ����ɤ�Ԥ��ޤ�.
//  ��ǽ : URI�ǥ����ɤ�Ԥ�
//  ���� : dst �Ѵ�����ʸ���ν񤭽Ф���.
//		  dst_len �Ѵ�����ʸ���ν񤭽Ф���κ���Ĺ.
//		  src �Ѵ�����ʸ��.
//		  src_len �Ѵ�����ʸ����Ĺ��.
// ���� : �ǥ����ɤ���ʸ���ο�(���Τޤޤ�ޤ�)
// **************************************************************************
int uri_decode(unsigned char *dst, unsigned int dst_len, const unsigned char *src, unsigned int src_len)
{

	int 		idx_src;
	int 		idx_dst;
	int 		cnt;
	char 		work[3];
	const char	*ptr_stop;
	char		*strtol_end_ptr;
	int 		code;


	// ���������å�
	if ((dst == NULL) || (dst_len < 1) || (src == NULL) || (src_len < 1))
	{
		return 0;
	}


	cnt = 0;
	ptr_stop = src;

	// =================
	// �ᥤ��롼��
	// =================
	for (idx_src = idx_dst = 0; (idx_src < src_len) && (idx_dst < dst_len) && (src[idx_src] != '\0'); idx_dst++ , cnt++)
	{
		if (src[idx_src] == '%')
		{
			if (idx_src + 2 > src_len)
			{
				break;
			}

			work[0] = src[idx_src+1];
			work[1] = src[idx_src+2];
			work[2] = '\0';

			code = strtol(work, &strtol_end_ptr, 16);
			ptr_stop = &src[idx_src + (strtol_end_ptr - work) + 1];

			if (code == LONG_MIN || code == LONG_MAX)
			{
				break;
			}

			if (strtol_end_ptr != NULL)
			{
				if (*strtol_end_ptr != '\0')
				{
					break;
				}
			}
			dst[idx_dst] = code;
			idx_src += 3;
		}
		else if ( src[idx_src] == '+' )
		{
			dst[idx_dst] = ' ';
			idx_src += 1;
			ptr_stop++;
		}
		else
		{
			dst[idx_dst] = src[idx_src];
			idx_src += 1;
			ptr_stop++;
		}

		if (idx_dst + 1 < dst_len)
		{
			dst[idx_dst + 1] = '\0';
		}
	}

	return cnt;
}

/********************************************************************************/
// "YYYY/MM/DD HH:MM:SS" �����θ��ߤ�������ʸ������������롣
/********************************************************************************/
void make_datetime_string(unsigned char *sentence)
{
	time_t				now;
	struct tm			*tm_p;

	// ���߻���Get.
	time(&now);
	tm_p = localtime(&now);

	sprintf(sentence, 	"%04d/%02d/%02d %02d:%02d:%02d",
						tm_p->tm_year+1900	,	// ǯ
						tm_p->tm_mon+1	,		// ��
						tm_p->tm_mday	,		// ��
						tm_p->tm_hour	,		// ����
						tm_p->tm_min	,		// ʬ
						tm_p->tm_sec		);	// ��

	return;
}


/********************************************************************************/
// time_t ���顢"YYYY/MM/DD HH:MM" ������ʸ������������롣
/********************************************************************************/
void conv_time_to_string(unsigned char *sentence, time_t conv_time)
{
	struct tm			*tm_p;

	if (conv_time == 0) {
		sprintf(sentence, "--/--/-- --:--");
		return ;
	}
	tm_p = localtime(&conv_time);
	sprintf(sentence, 	"%04d/%02d/%02d %02d:%02d",
						tm_p->tm_year+1900	,	// ǯ
						tm_p->tm_mon+1	,		// ��
						tm_p->tm_mday	,		// ��
						tm_p->tm_hour	,		// ����
						tm_p->tm_min		);	// ʬ

	return;
}




/********************************************************************************/
// time_t ���顢"YYYY/MM/DD" ������ʸ������������롣
/********************************************************************************/
void conv_time_to_date_string(unsigned char *sentence, time_t conv_time)
{
	struct tm			*tm_p;

	if (conv_time == 0) {
		sprintf(sentence, "--/--/--");
		return ;
	}
	tm_p = localtime(&conv_time);
	sprintf(sentence, 	"%04d/%02d/%02d",
						tm_p->tm_year+1900	,	// ǯ
						tm_p->tm_mon+1	,		// ��
						tm_p->tm_mday		);	// ��

	return;
}




/********************************************************************************/
// time_t ���顢"HH:MM" ������ʸ������������롣
/********************************************************************************/
void conv_time_to_time_string(unsigned char *sentence, time_t conv_time)
{
	struct tm			*tm_p;

	if (conv_time == 0) {
		sprintf(sentence, "--:--");
		return ;
	}
	tm_p = localtime(&conv_time);
	sprintf(sentence, 	"%02d:%02d",
						tm_p->tm_hour	,		// ����
						tm_p->tm_min		);	// ʬ

	return;
}


void conv_duration_to_string(unsigned char *sentence, dvd_duration *dvdtime)
{
	if (dvdtime == NULL) {
		sprintf(sentence, "--:--:--");
		return ;
	}
	if (dvdtime->hour == -1) {
		sentence[0]=0;
		return;
	}
	sprintf(sentence, "%d:%02d:%02d",
			dvdtime->hour,
			dvdtime->minute,
			dvdtime->second);

	return;
}


/********************************************************************************/
// 100000000 �� "100.00 MB" �ؤ��Ѵ���Ԥ���
//
//	K,M,G ���б���
/********************************************************************************/
void conv_num_to_unit_string(unsigned char *sentence, u_int64_t file_size)
{
	u_int64_t	real_size;
	u_int64_t	little_size;


	if ((int) file_size == -1) {
		sentence[0] = 0;
		return;
	} else if ( file_size < 1024 )
	{
		sprintf(sentence, "%lld B", file_size );
	}
	else if ( file_size < (1024 * 1024) )
	{
		real_size 	= file_size / 1024;
		little_size = (file_size * 100 / 1024) % 100;

		sprintf(sentence, "%lld.%02lld KB", real_size, little_size);
	}
	else if ( file_size < (1024 * 1024 * 1024) )
	{
		real_size 	= file_size / ( 1024*1024 );
		little_size = (file_size * 100 / (1024*1024)) % 100;

		sprintf(sentence, "%lld.%02lld MB", real_size, little_size);
	}
	else
	{

		real_size 	= file_size / ( 1024*1024*1024 );
		little_size = (file_size * 100 / (1024*1024*1024)) % 100;

		sprintf(sentence, "%lld.%02lld GB", real_size, little_size);
	}

	return;
}








//*******************************************************************
// �ǥХå����Ͻ����(�ե�����̾���å�)�ؿ�
// ���δؿ���ǽ�˸Ƥ֤ޤǤϡ��ǥХå����ϰ��ڽ��Ϥ���ʤ���
//*******************************************************************
void debug_log_initialize(const unsigned char *set_debug_log_filename)
{
	// ���������å�
	if (set_debug_log_filename == NULL)
		return;

	if ( strlen(set_debug_log_filename) == 0 )
		return;


	// �ǥХå����ե�����̾�򥻥åȡ�
	strncpy(debug_log_filename, set_debug_log_filename,	sizeof(debug_log_filename) );

	// �ǥХå��� �������λ�ե饰��0�ˡ�
	debug_log_initialize_flag = 0;

	return;
}



//*************************************************
// �ǥХå������Ѵؿ���
// printf() ��Ʊ���ե����ޥåȤˤƻ��Ѥ��롣
//*************************************************
void debug_log_output(char *fmt, ...)
{
	FILE	*fp;
	unsigned char	buf[1024*5+1];
	unsigned char	work_buf[1024*4+1];
	unsigned char	date_and_time[32];
	unsigned char	replase_date_and_time[48];
	va_list 	arg;
	int		len;

	// =========================================
	// �ǥХå��� ������ե饰������å�
	// =========================================
	if (debug_log_initialize_flag != 0 )
		return;


	// =========================================
	// Debug����ʸ����������
	// ��Ƭ�ˡ�date_and_time ���������Ƥ���
	// =========================================
	memset(buf, '\0', sizeof(buf));
	memset(work_buf, '\0', sizeof(work_buf));

	// ������Ϳ����줿ʸ�����Ÿ����
	va_start(arg, fmt);
	vsnprintf(work_buf, sizeof(work_buf), fmt, arg);
	va_end(arg);

	// work_buf�ΰ��ֺǸ��'\n'���Ĥ��Ƥ���������
	len = strlen(work_buf);
	if (work_buf[len-1] == '\n')
	{
		work_buf[len-1] = '\0';
	}

	// ������ʸ��������( "\ndate_and_time" �ˤʤ�)
	make_datetime_string(date_and_time);
	snprintf(replase_date_and_time, sizeof(replase_date_and_time), "\n%s[%d] ", date_and_time, getpid() );

	// ����ʸ�����������ϡ�
	snprintf(buf, sizeof(buf), "%s[%d] %s", date_and_time, getpid(), work_buf);
	replase_character(buf, sizeof(buf), "\n", replase_date_and_time);	// \n������date_and_time������

	// ���ֺǸ��'\n'��Ĥ��롣
	strncat(buf, "\n", sizeof(buf)-strlen(buf) );


	// =====================
	// ���ե��������
	// =====================

	// �ե����륪���ץ���ɵ��⡼��)
	fp = fopen(debug_log_filename, "a");
	if ( fp == NULL )
		return;

	// ����
	fwrite(buf, 1, strlen(buf), fp );	// ��å��������Τ����

	// �ե����륯����
	fclose( fp );

	return;
}



// **************************************************************************
// ��ĥ���ѹ��������ɲ��ѡ�
//	extension_convert_list�˽�����org �� rename �ؤ��Ѵ���Ԥ���
//
// ��) "hogehoge.m2p" �� "hogehoge.m2p.mpg"
// **************************************************************************
void extension_add_rename(unsigned char *rename_filename_p, size_t rename_filename_size)
{
	int i;
	unsigned char	ext[WIZD_FILENAME_MAX];

	if ( rename_filename_p == NULL )
		return;

	filename_to_extension(rename_filename_p, ext, sizeof(ext));

	if (strlen(ext) <= 0) return;

	for ( i=0; extension_convert_list[i].org_extension != NULL; i++ )
	{
		debug_log_output("org='%s', rename='%s'"
			, extension_convert_list[i].org_extension
			, extension_convert_list[i].rename_extension);

		// ��ĥ�Ұ��ס�
		if ( strcasecmp(ext, extension_convert_list[i].org_extension) == 0 )
		{
			debug_log_output(" HIT!!!" );
			// ��ĥ�Ҥ���ɲá�
			strncat(rename_filename_p, "."
				, rename_filename_size - strlen(rename_filename_p));
			strncat(rename_filename_p
				, extension_convert_list[i].rename_extension
				, rename_filename_size - strlen(rename_filename_p));

			debug_log_output("rename_filename_p='%s'", rename_filename_p);
			break;
		}
	}


	return;
}


// **************************************************************************
// ��ĥ���ѹ�����������ѡ�
//	extension_convert_list�˽�����rename �� org �ؤ��Ѵ���Ԥ���
//
// ��) "hogehoge.m2p.mpg" �� "hogehoge.m2p"
// **************************************************************************
void extension_del_rename(unsigned char *rename_filename_p, size_t rename_filename_size)
{
	int i;
	unsigned char	renamed_ext[WIZD_FILENAME_MAX];
	unsigned char	ext[WIZD_FILENAME_MAX];

	if ( rename_filename_p == NULL )
		return;

	for ( i=0; extension_convert_list[i].org_extension != NULL; i++ )
	{
		debug_log_output("org='%s', rename='%s'"
			, extension_convert_list[i].org_extension
			, extension_convert_list[i].rename_extension);

		snprintf(renamed_ext, sizeof(renamed_ext), ".%s.%s"
			, extension_convert_list[i].org_extension
			, extension_convert_list[i].rename_extension);

		// ��Ӥ����ĥ�Ҥ�Ʊ��Ĺ���ˤ����롣
		strncpy(ext, rename_filename_p, sizeof(ext));
		cat_before_n_length(ext, strlen(renamed_ext));

		// ��ĥ�Ұ��ס�
		if ( strcasecmp(ext, renamed_ext) == 0 )
		{
			debug_log_output(" HIT!!!" );
			// ��ĥ�Ҥ�ֺ����
			cat_after_n_length(rename_filename_p
				, strlen(extension_convert_list[i].rename_extension) + 1);

			debug_log_output("rename_filename_p='%s'", rename_filename_p);
			break;
		}
	}


	return;
}


// **************************************************************************
// * EUCʸ�������Ⱦ��ʸ�������Ѥˤ��롣
// **************************************************************************
void han2euczen(unsigned char *src, unsigned char *dist, int dist_size)
{

	unsigned char	*dist_p, *src_p;

	dist_p 	= dist;
	src_p	= src;

	while ( *src_p != '\0')
	{
		switch( (int)*src_p )
		{
			case ' ':	strncpy(dist_p, "  ", dist_size);	dist_p+=2;	break;
			case '!':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '"':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '#':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '$':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '%':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '&':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '\'':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '(':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case ')':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '*':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '+':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case ',':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '-':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '.':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '/':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '0':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '1':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '2':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '3':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '4':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '5':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '6':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '7':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '8':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '9':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case ':':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '<':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '=':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '>':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '?':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '@':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'A':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'B':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'C':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'D':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'E':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'F':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'G':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'H':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'I':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'J':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'K':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'L':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'M':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'N':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'O':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'P':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'Q':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'R':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'S':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'T':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'U':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'V':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'W':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'X':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'Y':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'Z':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '[':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '\\':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case ']':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '^':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '_':	strncpy(dist_p, "  ", dist_size);	dist_p+=2;	break;
			case '`':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'a':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'b':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'c':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'd':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'e':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'f':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'g':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'h':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'i':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'j':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'k':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'l':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'm':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'n':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'o':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'p':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'q':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'r':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 's':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 't':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'u':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'v':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'w':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'x':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'y':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case 'z':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '{':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '|':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;
			case '}':	strncpy(dist_p, "��", dist_size);	dist_p+=2;	break;

			default:
				*dist_p = *src_p;
				dist_p++;
				*dist_p = '\0';
				break;
		}

		src_p++;
	}

	return;
}

//******************************************************************
// EUCʸ�����n byte�˺����ʸ���󶭳��򸫤��ɤ��˷פ餦��
// �⤷ menu_font_metric �����ꤵ��Ƥ��� menu_font_metric_string ��
// ���ꤵ��Ƥ���С������ͤ˽��äƤ���󤮤롣
//******************************************************************
void euc_string_cut_n_length(unsigned char *euc_sentence,  unsigned int n)
{
	int	euc_flag = 0;
	unsigned char	*p, *prev_p;
	int total_len = 0;
	int size = 0;

	total_len = global_param.menu_font_metric * n;


	prev_p = p = euc_sentence;
	while (*p && total_len > 0) {
		if (*p >= sizeof(global_param.menu_font_metric_string)) {
			size = global_param.menu_font_metric;
		} else {
			char ch;
			ch = global_param.menu_font_metric_string[*p];
			if (ch < 'a' || ch > 'z') {
				size = global_param.menu_font_metric;
			} else {
				size = ch - 'a';
				if (size <= 0) {
					//debug_log_output("damn. old size: %d", size);
					size = global_param.menu_font_metric;
				}
			}
		}
		if (total_len < size) {
			if (euc_flag == 1) {
				*prev_p = '\0';
			} else {
				*p = '\0';
			}
			return ;
		}
		if (euc_flag == 0 && (( *p >= 0xA1 ) || ( *p == 0x8E ))) {
			// EUC-Code or Ⱦ�ѥ������ʤξ��
			euc_flag = 1;
		} else {
			euc_flag = 0;
		}
		total_len -= size;
		prev_p = p++;
	}

	debug_log_output("debug: rest %d", total_len);
	*p = '\0';

	return;
}
// **************************************************************************
// * PNG�ե����ޥåȥե����뤫�顢���������������롣
// **************************************************************************
void png_size(unsigned char *png_filename, unsigned int *x, unsigned int *y)
{
	int		fd;
	unsigned char	buf[255];
	ssize_t	read_len;

	*x = 0;
	*y = 0;

	fd = open(png_filename, O_RDONLY);
	if ( fd < 0 )
	{
		return;
	}

	// �إå�+������(0x18byte)  �ɤ�
	memset(buf, 0, sizeof(buf));
	read_len = read(fd, buf, 0x18);
	if ( read_len == 0x18)
	{
		*x = 	(buf[0x10] << 24)	+
				(buf[0x11] << 16)	+
				(buf[0x12] << 8 )	+
				(buf[0x13]);

		*y = 	(buf[0x14] << 24) 	+
				(buf[0x15] << 16)	+
				(buf[0x16] << 8 )	+
				(buf[0x17]);
	}

	close( fd );


	return;
}


// **************************************************************************
// * GIF�ե����ޥåȥե����뤫�顢���������������롣
// **************************************************************************
void gif_size(unsigned char *gif_filename, unsigned int *x, unsigned int *y)
{
	int		fd;
	unsigned char	buf[255];
	ssize_t	read_len;

	*x = 0;
	*y = 0;

	fd = open(gif_filename, O_RDONLY);
	if ( fd < 0 )
	{
		return;
	}


	// �إå�+������(10byte)  �ɤ�
	memset(buf, 0, sizeof(buf));
	read_len = read(fd, buf, 10 );
	if ( read_len == 10)
	{
		*x = buf[6] + (buf[7] << 8);
		*y = buf[8] + (buf[9] << 8);
	}

	close( fd );

	return;
}



// **************************************************************************
// * JPEG�ե����ޥåȥե����뤫�顢���������������롣
// **************************************************************************
void  jpeg_size(unsigned char *jpeg_filename, unsigned int *x, unsigned int *y)
{
	int		fd;
	unsigned char	buf[255];
	ssize_t		read_len;
	off_t		length;

	*x = 0;
	*y = 0;

	//debug_log_output("jpeg_size: '%s'.", jpeg_filename);


	fd = open(jpeg_filename,  O_RDONLY);
	if ( fd < 0 )
	{
		return;
	}

	while ( 1 )
	{
		// �ޡ���(2byte)  �ɤ�
		read_len = read(fd, buf, 2);
		if ( read_len != 2)
		{
			//debug_log_output("fraed() EOF.\n");
			break;
		}

		// Start of Image.
		if (( buf[0] == 0xFF ) && (buf[1] == 0xD8))
		{
			continue;
		}

		// Start of Frame ����
		if (( buf[0] == 0xFF ) && ( buf[1] >= 0xC0 ) && ( buf[1] <= 0xC3 )) // SOF ����
		{
			//debug_log_output("SOF0 Detect.");

			// sof �ǡ����ɤ߹���
			memset(buf, 0, sizeof(buf));
			read_len = read(fd, buf, 0x11);
			if ( read_len != 0x11 )
			{
				debug_log_output("fraed() error.\n");
				break;
			}

			*y = (buf[3] << 8) + buf[4];
			*x = (buf[5] << 8) + buf[6];

			break;
		}

		// SOS����
		if (( buf[0] == 0xFF ) && (buf[1] == 0xDA)) // SOS ����
		{
			//debug_log_output("Start Of Scan.\n");

			// 0xFFD9 õ����
			while ( 1 )
			{
				// 1byte �ɤ�
				read_len = read(fd, buf, 1);
				if ( read_len != 1 )
				{
					//debug_log_output("fraed() error.\n");
					break;
				}

				// 0xFF���ä��顢�⤦1byte�ɤ�
				if ( buf[0] == 0xFF )
				{
					buf[0] = 0;
					read(fd, buf, 1);

					// 0xD9���ä��� ��λ
					if ( buf[0] == 0xD9 )
					{
						//debug_log_output("End Of Scan.\n");
						break;
					}
				}
			}
			continue;
		}


		// length �ɤ�
		memset(buf, 0, sizeof(buf));
		read(fd, buf, 2);
		length = (buf[0] << 8) + buf[1];

		// lengthʬ�ȤФ�
		lseek(fd, length-2, SEEK_CUR );
	}

	close(fd);

	return;
}



// **************************************************************************
// * SJISʸ������椫�顢code���ޤ�ʸ��(2byteʸ��)�򡢸���ʸ������ִ����롣
// **************************************************************************
void sjis_code_thrust_replase(unsigned char *sentence, const unsigned char code)
{
	int i;
	unsigned char rep_code[2];

	// �ִ�����ʸ����(SJIS��'��'��
	rep_code[0] = 0x81;
	rep_code[1] = 0x96;

	// ʸ����Ĺ����1byte�ʲ��ʤ顢����ɬ�פʤ���
	if ( strlen(sentence) <= 1 )
	{
		return;
	}


	// �ִ��о�ʸ�� �ܺ�
	for ( i=1; sentence[i] != '\0' ;i++ )
	{
		// code �˥ҥåȡ�
		if ( sentence[i] == code )
		{
			// 1byte������SJIS 1byte�ܤ��ϰϡ�(0x81��0x9F��0xE0����0xFC)
			if ((( sentence[i-1] >= 0x81 ) && (sentence[i-1] <= 0x9F)) ||
				(( sentence[i-1] >= 0xE0 ) && (sentence[i-1] <= 0xFC))		)
			{

				debug_log_output("SJIS Replase HIT!!!!");

				// �ִ��¹�
				sentence[i-1] = rep_code[0];
				sentence[i  ] = rep_code[1];
			}
		}
	}

	return;

}


/* Cygwin���� strcasestr ���ʤ��Τ� strncasecmp��ȤäƼ��� */
char *my_strcasestr(const char *p1, const char *p2)
{
	size_t len;

	len = strlen(p2);
	if (len == 0) return (char*)p1;

	while (*p1) {
		if (!strncasecmp(p1, p2, len)) {
			return (char*)p1;
		}
		p1++;
	}
	return NULL;
}


char *path_sanitize(char *orig_dir, size_t dir_size)
{
	char *p;
	char *q, *dir;
	char *buf;
	size_t malloc_len;

	if (orig_dir == NULL) return NULL;

	malloc_len = strlen(orig_dir) * 2;
	buf = malloc(malloc_len);
	buf[0] = '\0';
	p = buf;

	dir = q = orig_dir;
	while (q != NULL) {
		dir = q;
		while (*dir == '/') dir ++;

		q = strchr(dir, '/');
		if (q != NULL) {
			*q++ = '\0';
		}

		if (!strcmp(dir, "..")) {
			p = strrchr(buf, '/');
			if (p == NULL) {
				free(buf);
				dir[0] = '\0';
				return NULL; //  not allowed.
			}
			*p = '\0';
		} else if (strcmp(dir, ".")) {
			p += snprintf(p, malloc_len - (p - buf), "/%s", dir);
		}
	}

	if (buf[0] == '\0') {
		strncpy(orig_dir, "/", dir_size);
	} else {
		strncpy(orig_dir, buf, dir_size);
	}
	free(buf);
	return orig_dir;
}

void send_printf(int fd, char *fmt, ...) {
	va_list ap;
	char linebuf[2048];

	va_start(ap, fmt);
	vsnprintf(linebuf, sizeof(linebuf), fmt, ap);
	va_end(ap);

	write(fd, linebuf, strlen(linebuf));
}

// hogehoge/abc/ddd/// -> hogehoge/abc/
char *get_parent_path(char *dst, char *src, size_t len)
{
	char *p;

	if (strlen(src) > len || len <= 0) {
		debug_log_output("get_parent_path: no enough space");
		strncpy(dst, "/", len);
		return dst;
	}
	// �Ȥꤢ�����������ԡ���
	strncpy(dst, src, len);

	// ���ָ��� '/' ����
	for (p = dst + strlen(dst) - 1; p > dst && *p == '/'; p--) *p = '\0';

	p = strrchr(dst, '/');
	if (p == NULL || p == dst) {
		debug_log_output("get_parent_path: no / anymore..., i.e., root.");
		strncpy(dst, "/", len);
		return dst;
	}

	// �Ĥä��Ǹ�� '/' ��������
	*++p = '\0';

	return dst;
}
