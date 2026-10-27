#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "section_parser.h"
#include "utils.h"

namespace parser {



// debug
static const char *s_szSectState [EN_SECTION_STATE_NUM] = {
	"INIT",
	"RECEIVING",
	"RECEIVED",
	"COMPLETE",
};

section_info::section_info (uint8_t *p_buff, size_t size, EN_SECTION_TYPE type)
	:raw_ (NULL)
	,data_ (NULL)
	,tail_ (NULL)
	,state_ (EN_SECTION_STATE__INIT)
	,type_ (EN_SECTION_TYPE__PSISI)
	,next_ (NULL)
{
	memset (&sect_hdr_, 0x00, sizeof(sect_hdr_));

	if (p_buff && (size > 0)) {
		raw_ = p_buff;
		tail_ = p_buff + size;
		parse_header ();
	}

	type_ = type;
}

section_info::section_info (section_info *p_obj)
	:raw_ (NULL)
	,data_ (NULL)
	,tail_ (NULL)
	,state_ (EN_SECTION_STATE__INIT)
	,type_ (EN_SECTION_TYPE__PSISI)
	,next_ (NULL)
{
	if (!p_obj) {
		return;
	}

	memcpy (&sect_hdr_, &(p_obj->sect_hdr_), sizeof(sect_hdr_));
	raw_ = p_obj->raw_;
	data_ = p_obj->data_;
	tail_ = p_obj->tail_;
	state_ = p_obj->state_;
	next_ = p_obj->next_;
	type_ = p_obj->type_;
}

section_info::~section_info (void)
{
}

/**
 *
 */
uint8_t *section_info::get_header_addr (void) const
{
	return raw_;
}

/**
 *
 */
const ST_SECTION_HEADER *section_info::get_header (void) const
{
	return &sect_hdr_;
}

/**
 *
 */
uint8_t *section_info::get_data_part_addr (void) const
{
	return data_;
}

/**
 *
 */
uint16_t section_info::get_data_part_len (void) const
{
	if (type_ == EN_SECTION_TYPE__PSISI) {
		if (sect_hdr_.section_syntax_indicator == 0) {
			// short format section header
			return sect_hdr_.section_length - SECTION_CRC32_LEN;
		} else {
			return sect_hdr_.section_length - SECTION_HEADER_FIX_LEN - SECTION_CRC32_LEN;
		}

	} else if (type_ == EN_SECTION_TYPE__DSMCC) {
		return sect_hdr_.section_length - SECTION_HEADER_FIX_LEN - SECTION_CRC32_LEN;

	} else {
		_UTL_LOG_E ("EN_SECTION_TYPE is invalid.");
		return 0;
	}
}

/**
 *
 */
void section_info::parse_header (void)
{
	if (!raw_ || !tail_) {
		return;
	}

	data_ = parse_header (&sect_hdr_, raw_, tail_);
}

/**
 *
 */
uint8_t * section_info::parse_header (ST_SECTION_HEADER *p_hdr_out, uint8_t *p_start, uint8_t *p_end, EN_SECTION_TYPE en_type) 
{
	if (!p_hdr_out || !p_start || !p_end) {
		return NULL;
	}

	int size = 0;
	uint8_t *p = NULL; //work

	size = p_end - p_start;
	if (size < SECTION_SHORT_HEADER_LEN) {
		// need more data
		return NULL;
	}

	p = p_start;
	p_hdr_out->table_id                 =   *(p+0);
	p_hdr_out->section_syntax_indicator =  (*(p+1) >> 7) & 0x01;
	p_hdr_out->private_indicator        =  (*(p+1) >> 6) & 0x01;
	p_hdr_out->section_length           = ((*(p+1) << 8) | *(p+2)) & 0x0fff;

	if (en_type == EN_SECTION_TYPE__PSISI) {
		if (p_hdr_out->section_syntax_indicator == 0) {
			// short format section header
			return p + SECTION_SHORT_HEADER_LEN;

		} else {
			// long format section header

			if (size < SECTION_HEADER_LEN) {
				// need more data
				return NULL;
			}

			p_hdr_out->table_id_extension     = ((*(p+3) << 8) | *(p+4));
			p_hdr_out->version_number         =  (*(p+5) >> 1) & 0x1f;
			p_hdr_out->current_next_indicator =   *(p+5)       & 0x01;
			p_hdr_out->section_number         =   *(p+6);
			p_hdr_out->last_section_number    =   *(p+7);

			return p + SECTION_HEADER_LEN;
		}

	} else if (en_type == EN_SECTION_TYPE__DSMCC) {
		// long format section header

		if (size < SECTION_HEADER_LEN) {
			// need more data
			return NULL;
		}

		p_hdr_out->table_id_extension     = ((*(p+3) << 8) | *(p+4));
		p_hdr_out->version_number         =  (*(p+5) >> 1) & 0x1f;
		p_hdr_out->current_next_indicator =   *(p+5)       & 0x01;
		p_hdr_out->section_number         =   *(p+6);
		p_hdr_out->last_section_number    =   *(p+7);

		return p + SECTION_HEADER_LEN;

	} else {
		_UTL_LOG_E ("EN_SECTION_TYPE is invalid.");
		return NULL;
	}
}

/**
 *
 */
void section_info::dump_header (void) const
{
	if (type_ == EN_SECTION_TYPE__PSISI) {
		dump_header (&sect_hdr_, (sect_hdr_.section_syntax_indicator == 0) ? true : false);

	} else if (type_ == EN_SECTION_TYPE__DSMCC) {
		dump_header (&sect_hdr_, false);

	} else {
		_UTL_LOG_E ("EN_SECTION_TYPE is invalid.");
	}
}

/**
 *
 */
void section_info::dump_header (const ST_SECTION_HEADER *p_hdr, bool is_short_format) 
{
	if (!p_hdr) {
		return ;
	}

	if (is_short_format) {
		_UTL_LOG_I (
			"SectHeader: tbl_id:[0x%02x] syntax:[0x%02x] priv:[0x%02x] len:[%d]\n",
			p_hdr->table_id,
			p_hdr->section_syntax_indicator,
			p_hdr->private_indicator,
			p_hdr->section_length
		);
	} else {
		_UTL_LOG_I (
			"SectHeader: tbl_id:[0x%02x] syntax:[0x%02x] priv:[0x%02x] len:[%d] tbl_ext:[0x%04x] ver:[0x%02x] next:[0x%02x] num:[0x%02x] last:[0x%02x]\n",
			p_hdr->table_id,
			p_hdr->section_syntax_indicator,
			p_hdr->private_indicator,
			p_hdr->section_length,
			p_hdr->table_id_extension,
			p_hdr->version_number,
			p_hdr->current_next_indicator,
			p_hdr->section_number,
			p_hdr->last_section_number
		);
	}
}

/**
 * is_receive_all
 * 当該sectionがすべて受け取れたかどうか
 */
bool section_info::is_receive_all (void) const
{
	if (!raw_ || !tail_) {
		return false;
	}

	// 右辺はheader含めたsectionの合計
	if ((tail_ - raw_) < (sect_hdr_.section_length + SECTION_SHORT_HEADER_LEN)) {
		_UTL_LOG_D ("section fragment\n");
		return false;
	}

	return true;
}

/**
 * truncate
 */
size_t section_info::truncate (void)
{
	if (!is_receive_all ()) {
		_UTL_LOG_E ("can truncate only if is_receive_all is true");
		return 0;
	}
	size_t len = (tail_ - raw_) - (sect_hdr_.section_length + SECTION_SHORT_HEADER_LEN);
	tail_ -= len;
	return len;
}

/**
 * clear
 */
void section_info::clear (void)
{
	memset (&sect_hdr_, 0x00, sizeof(ST_SECTION_HEADER));
	type_ = EN_SECTION_TYPE__PSISI;
	state_ = EN_SECTION_STATE__INIT;
	raw_ = NULL;
	data_ = NULL;
	tail_ = NULL;
	next_ = NULL;
}

/**
 *
 */
bool section_info::check_crc32 (void) const
{
	return ts_arib_common::crc32(raw_, sect_hdr_.section_length + SECTION_SHORT_HEADER_LEN) == 0 ? true : false;
}


const uint32_t section_parser::POOL_SIZE = 4096;

section_parser::section_parser (EN_SECTION_TYPE type)
	:pid_ (0)
	,sect_list_top_(NULL)
	,sect_list_bottom_ (NULL)
	,list_is_fifo_ (false)
	,fifo_num_ (0)
	,work_sect_info_ (NULL)
	,pool_ (NULL)
	,pool_ind_ (0)
	,pool_size_ (POOL_SIZE)
	,type_ (EN_SECTION_TYPE__PSISI)
	,is_async_delete_ (false)
	,is_ignore_section_ (false)
{
	pool_ = new uint8_t [pool_size_];
	memset (pool_, 0x00, pool_size_);
	type_ = type;
}

section_parser::section_parser (int fifo_num, EN_SECTION_TYPE type)
	:pid_ (0)
	,sect_list_top_(NULL)
	,sect_list_bottom_ (NULL)
	,list_is_fifo_ (false)
	,fifo_num_ (0)
	,work_sect_info_ (NULL)
	,pool_ (NULL)
	,pool_ind_ (0)
	,pool_size_ (POOL_SIZE)
	,type_ (EN_SECTION_TYPE__PSISI)
	,is_async_delete_ (false)
	,is_ignore_section_ (false)
{
	pool_ = new uint8_t [pool_size_];
	memset (pool_, 0x00, pool_size_);

	if (fifo_num > 0) {
		list_is_fifo_ = true;
		fifo_num_ = fifo_num;
	}

	type_ = type;
}

section_parser::section_parser (size_t pool_size, EN_SECTION_TYPE type)
	:pid_ (0)
	,sect_list_top_(NULL)
	,sect_list_bottom_ (NULL)
	,list_is_fifo_ (false)
	,fifo_num_ (0)
	,work_sect_info_ (NULL)
	,pool_ (NULL)
	,pool_ind_ (0)
	,pool_size_ (POOL_SIZE)
	,type_ (EN_SECTION_TYPE__PSISI)
	,is_async_delete_ (false)
	,is_ignore_section_ (false)
{
	pool_size_ = pool_size;
	pool_ = new uint8_t [pool_size_];
	memset (pool_, 0x00, pool_size_);
	type_ = type;
}

section_parser::section_parser (size_t pool_size ,int fifo_num, EN_SECTION_TYPE type)
	:pid_ (0)
	,sect_list_top_(NULL)
	,sect_list_bottom_ (NULL)
	,list_is_fifo_ (false)
	,fifo_num_ (0)
	,work_sect_info_ (NULL)
	,pool_ (NULL)
	,pool_ind_ (0)
	,pool_size_ (POOL_SIZE)
	,type_ (EN_SECTION_TYPE__PSISI)
	,is_async_delete_ (false)
	,is_ignore_section_ (false)
{
	pool_size_ = pool_size;
	pool_ = new uint8_t [pool_size_];
	memset (pool_, 0x00, pool_size_);

	if (fifo_num > 0) {
		list_is_fifo_ = true;
		fifo_num_ = fifo_num;
	}

	type_ = type;
}

section_parser::~section_parser (void)
{
	delete [] pool_;
	pool_ = NULL;
}

/**
 *
 */
section_info* section_parser::attach_section_list (uint8_t *p_buff, size_t size)
{
	if (!p_buff || (size == 0)) {
		return NULL;
	}

	section_info* p_rtn = NULL;

	if ((pool_size_ - pool_ind_) < size) {
		// pool full: 最古(先頭)の COMPLETE セクションを退避して領域を空ける (自己回復)。
		// 完了済みセクションは同一再送の検知用に保持しているだけなので、
		// 空きが必要になったら古いものから捨ててよい (上位は version 単位で重複排除する)。
		while ((pool_size_ - pool_ind_) < size) {
			if (!evict_oldest_section ()) {
				// 退避できる COMPLETE セクションが無い (先頭が受信中など)
				_UTL_LOG_E ("pid:[0x%04x] pool full!!  pool_size_:[%lu] pool_ind_:[%lu] remain:[%lu] attach_size:[%lu]", pid_, pool_size_, pool_ind_, pool_size_ - pool_ind_, size);
				return NULL;
			}
		}
	}


	uint8_t* p_cur = pool_ + pool_ind_;

	// sectionの生データをpoolにコピー
	memcpy (p_cur, p_buff, size);
	pool_ind_ += size;


	if (work_sect_info_ && (work_sect_info_->state_ == EN_SECTION_STATE__RECEIVING)) {
		// follow
		if (p_cur != work_sect_info_->tail_) {
			_UTL_LOG_E ("BUG: pid:[0x%04x] (p_cur != work_sect_info_->tail_)", pid_);
		}
		work_sect_info_->tail_ = p_cur + size;
		p_rtn = work_sect_info_;

	} else {
		// first
		section_info tmp_sect (p_cur, size, type_);

		section_info* p_list = add_section_list (&tmp_sect);
		if (!p_list) {

			// cancel pool data
//TODO memcpy-param-overlap
//			memcpy (p_cur, pool_ + pool_ind_, (pool_ + (pool_size_ -1)) - (pool_ + pool_ind_));
			std::vector<uint8_t> tmp (pool_size_, 0);
			size_t cpsize = (pool_ + pool_size_) - (pool_ + pool_ind_);
			memcpy (tmp.data(), pool_ + pool_ind_, cpsize);
			memcpy (p_cur, tmp.data(), cpsize);

			pool_ind_ -= size;

			p_rtn = NULL;

		} else {
			p_rtn = p_list;
		}
	}

	return p_rtn;
}

/**
 *
 */
section_info* section_parser::add_section_list (section_info *p_sect_info)
{
	if (!p_sect_info) {
		return NULL;
	}


	section_info *p_tmp = new section_info (p_sect_info);
	p_tmp->next_ = NULL;

	if (!sect_list_top_) {
		// リストが空
		sect_list_top_ = p_tmp;
		sect_list_bottom_ = p_tmp;

	} else {
		// 末尾につなぎます
		sect_list_bottom_->next_ = p_tmp;
		sect_list_bottom_ = p_tmp;
	}

	return p_tmp;
}

/**
 * detach_section_list
 * 指定のセクションを削除します
 *
 * TODO 
 * これを呼ぶところは今取得中のセクションで最後尾にあたるので
 * raw_, data_, tail_のshiftは不要とします
 */
void section_parser::detach_section_list (const section_info *p_sect_info)
{
	if (!p_sect_info) {
		return ;
	}


	delete_section_list (p_sect_info, false);

	// pool から消す - 間詰める
//TODO memcpy-param-overlap
//	memcpy (p_sect_info->raw_, p_sect_info->tail_, pool_ + (pool_size_ -1) - p_sect_info->tail_);
	std::vector<uint8_t> tmp (pool_size_, 0);
	size_t cpsize = pool_ + pool_size_ - p_sect_info->tail_;
	memcpy (tmp.data(), p_sect_info->tail_, cpsize);
	memcpy (p_sect_info->raw_, tmp.data(), cpsize);

	size_t shift = p_sect_info->tail_ - p_sect_info->raw_;
	pool_ind_ -= shift;

	// pool 消す時はセットで
	if (p_sect_info == work_sect_info_) {
// delete_section_list内で実行済
//		work_sect_info_->clear ();
		work_sect_info_ = NULL;
	}

	delete p_sect_info;
}

/**
 * delete_section_list
 * インスタンスの一致で削除
 * 重複していないこと前提
 */
void section_parser::delete_section_list (const section_info &sect_info, bool is_delete)
{
	section_info *p_tmp = NULL;
	section_info *p_bef = NULL;
	section_info *p_del = NULL;

	if (!sect_list_top_) {
		// リストが空

	} else {
		p_tmp = sect_list_top_;

		while (p_tmp) {
			if (*p_tmp == sect_info) {
				// 一致

				// 後でfreeするため保存
				p_del = p_tmp;

				if (p_tmp == sect_list_top_) {
					// topの場合

					if (p_tmp->next_) {
						sect_list_top_ = p_tmp->next_;
						p_tmp->next_ = NULL;
					} else {
						// リストが１つ -> リストが空になるはず
						sect_list_top_ = NULL;
						sect_list_bottom_ = NULL;
						p_tmp->next_ = NULL;
					}

				} else if (p_tmp == sect_list_bottom_) {
					// bottomの場合

					if (p_bef) {
						// 切り離します
						p_bef->next_ = NULL;
						sect_list_bottom_ = p_bef;
					}

				} else {
					// 切り離します
					p_bef->next_ = p_tmp->next_;
					p_tmp->next_ = NULL;
				}

				if (is_delete) {
					_UTL_LOG_D ("delete");
					p_del->clear();
					delete p_del;
					p_del = NULL;
				}

				break;

			} else {
				// ひとつ前のアドレス保存
				p_bef = p_tmp;
			}

			// next set
			p_tmp = p_tmp->next_;
		}
	}
}

/**
 * delete_section_list
 * アドレスの一致で削除
 * 重複していないこと前提
 */
void section_parser::delete_section_list (const section_info *p_sect_info, bool is_delete)
{
	if (!p_sect_info) {
		return ;
	}

	section_info *p_tmp = NULL;
	section_info *p_bef = NULL;
	section_info *p_del = NULL;

	if (!sect_list_top_) {
		// リストが空

	} else {
		p_tmp = sect_list_top_;

		while (p_tmp) {
			if (p_sect_info == p_tmp) {
				// 一致

				// 後でfreeするため保存
				p_del = p_tmp;

				if (p_tmp == sect_list_top_) {
					// topの場合

					if (p_tmp->next_) {
						sect_list_top_ = p_tmp->next_;
						p_tmp->next_ = NULL;
					} else {
						// リストが１つ -> リストが空になるはず
						sect_list_top_ = NULL;
						sect_list_bottom_ = NULL;
						p_tmp->next_ = NULL;
					}

				} else if (p_tmp == sect_list_bottom_) {
					// bottomの場合

					if (p_bef) {
						// 切り離します
						p_bef->next_ = NULL;
						sect_list_bottom_ = p_bef;
					}

				} else {
					// 切り離します
					p_bef->next_ = p_tmp->next_;
					p_tmp->next_ = NULL;
				}

				if (is_delete) {
					_UTL_LOG_D ("delete");
					p_del->clear();
					delete p_del;
					p_del = NULL;
				}

				break;

			} else {
				// ひとつ前のアドレス保存
				p_bef = p_tmp;
			}

			// next set
			p_tmp = p_tmp->next_;
		}
	}
}

/**
 *
 */
void section_parser::detach_all_section_list (void)
{
	delete_all_section_list ();

	if (pool_) {
		memset (pool_, 0x00, pool_size_);
		pool_ind_ = 0;
	}

	// pool 消す時はセットで
	if (work_sect_info_) {
// delete_all_section_list内で実行済
//		work_sect_info_->clear ();
		work_sect_info_ = NULL;
	}
}

/**
 * delete_all_section_list
 * リスト先頭から順に削除
 */
void section_parser::delete_all_section_list (void)
{
	section_info *p_tmp = NULL;
	section_info *p_del = NULL;


	if (!sect_list_top_) {
		// リストが空

	} else {
		p_tmp = sect_list_top_;

		while (p_tmp) {
			p_del = p_tmp;

			// next set
			p_tmp = p_tmp->next_;

			p_del->clear();
			delete p_del;
			p_del = NULL;
		}
	}

	sect_list_top_ = NULL;
	sect_list_bottom_ = NULL;
}

/**
 * shift_all_section_list
 * リストのsectionのmpRaw,data_,tail_をずらす
 */
void section_parser::shift_all_section_list (size_t shift_size)
{
	section_info *p_tmp = NULL;


	if (!sect_list_top_) {
		// リストが空

	} else {
		p_tmp = sect_list_top_;

		while (p_tmp) {

			p_tmp->raw_ -= shift_size;
			p_tmp->data_ -= shift_size;
			p_tmp->tail_ -= shift_size;

			// next set
			p_tmp = p_tmp->next_;
		}
	}
}

/**
 * search_section_list
 * リスト先頭から検索 (登録順で古いものから)
 * インスタンスの一致
 */
section_info *section_parser::search_section_list (const section_info &sect_info) const
{
	section_info *p_tmp = NULL;


	if (!sect_list_top_) {
		// リストが空

	} else {
		p_tmp = sect_list_top_;

		while (p_tmp) {
			if (*p_tmp == sect_info) {
				// 一致
				return p_tmp;
			}

			// next set
			p_tmp = p_tmp->next_;
		}
	}

	// not found
	return NULL;
}

/**
 *
 */
void section_parser::check_detach_fifo_section_list (void)
{
	if (!list_is_fifo_) {
		return;
	}

	if (fifo_num_ >= get_section_list_num ()) {
		return;
	}

	section_info *p_del_sect = check_delete_fifo_section_list ();
	if (p_del_sect) {
		// pool から消す - 間詰める
//TODO memcpy-param-overlap
//		memcpy (p_del_sect->raw_, p_del_sect->tail_, pool_ + (pool_size_ -1) - p_del_sect->tail_);
		std::vector<uint8_t> tmp (pool_size_, 0);
		size_t cpsize = pool_ + pool_size_ - p_del_sect->tail_;
		memcpy (tmp.data(), p_del_sect->tail_, cpsize);
		memcpy (p_del_sect->raw_, tmp.data(), cpsize);

		uint32_t shift = p_del_sect->tail_ - p_del_sect->raw_;
		pool_ind_ -= shift;

		shift_all_section_list (shift);

		// pool 消す時はセットで
		if (p_del_sect == work_sect_info_) {
			work_sect_info_->clear ();
			work_sect_info_ = NULL;
		}

		delete p_del_sect;
		p_del_sect = NULL;
	}
}

/**
 * 最も古いもの(top)のアドレスをリストから切り離し、そのアドレスを返します。
 */
section_info* section_parser::check_delete_fifo_section_list (void)
{
	section_info *p_del = NULL;


	if (!sect_list_top_) {
		// リストが空

	} else {
		if (!sect_list_top_->next_) {
			// リストが一つ
			p_del = sect_list_top_;

			sect_list_top_ = NULL;
			sect_list_bottom_ = NULL;

		} else {
			p_del = sect_list_top_;
			sect_list_top_ = sect_list_top_->next_;
		}
	}

	return p_del;
}

/**
 * evict_oldest_section
 * pool full 時に、最古(先頭=最低アドレス)の COMPLETE セクションを1つ退避して
 * pool を間詰めする。先頭が受信中(未完了)の場合は中間退避となり pool 圧縮の
 * 前提(先頭から詰める)が崩れるため退避しない。
 * 退避できたら true、できなければ false。
 */
bool section_parser::evict_oldest_section (void)
{
	if (!sect_list_top_) {
		return false;
	}

	// pool は追加順=アドレス順なので先頭が最低アドレス。圧縮が成立するのは先頭退避のみ。
	section_info *p_del = sect_list_top_;

	// 受信中のセクションは退避しない (再組立中のため)
	if (p_del->state_ != EN_SECTION_STATE__COMPLETE) {
		return false;
	}

	// リストの先頭から切り離す
	section_info *p_unlinked = check_delete_fifo_section_list ();
	if (p_unlinked != p_del) {
		// 想定外 (先頭以外が返った)
		return false;
	}

	// pool から消す - 間詰める (check_detach_fifo_section_list と同じ圧縮)
	std::vector<uint8_t> tmp (pool_size_, 0);
	size_t cpsize = pool_ + pool_size_ - p_del->tail_;
	memcpy (tmp.data(), p_del->tail_, cpsize);
	memcpy (p_del->raw_, tmp.data(), cpsize);

	uint32_t shift = p_del->tail_ - p_del->raw_;
	pool_ind_ -= shift;

	// 残りのセクションのアドレスをずらす
	shift_all_section_list (shift);

	// pool 消す時はセットで
	if (p_del == work_sect_info_) {
		work_sect_info_ = NULL;
	}

	delete p_del;

	return true;
}

/**
 *
 */
int section_parser::get_section_list_num (void) const
{
	section_info *p_tmp = NULL;
	int n = 0;

	p_tmp = sect_list_top_;
	while (p_tmp) {
		// next set
		p_tmp = p_tmp->next_;
		++ n ;
	}

	return n;
}

/**
 *
 */
uint16_t section_parser::get_pid (void) const
{
	return pid_;
}

/**
 *
 */
void section_parser::dump_section_list (void) const
{
	_UTL_LOG_I ("- section list ----------");
	section_info *p_tmp = NULL;
	int n = 0;

	p_tmp = sect_list_top_;
	while (p_tmp) {

		_UTL_LOG_I (
			" %d: table_id:[0x%02x] state:[%s] tail-raw:[%d] (%p -> %p)\n",
			n,
			p_tmp->sect_hdr_.table_id,
			s_szSectState[p_tmp->state_],
			int(p_tmp->tail_ - p_tmp->raw_),
			p_tmp,
			p_tmp->next_
		);

		// next set
		p_tmp = p_tmp->next_;
		++ n ;
	}
	_UTL_LOG_I ("-------------------------");
}

/**
 *
 */
EN_CHECK_SECTION section_parser::check_section_first (uint8_t *p_payload, size_t payload_size)
{
	if (!p_payload || (payload_size == 0)) {
		return EN_CHECK_SECTION__INVALID;
	}

	uint8_t *p = p_payload;
	size_t size = payload_size;

	uint8_t pointer_field = *p;
	p += 1;
	size = size -1;


	if((p + pointer_field) >= (p_payload + payload_size)) {
//TODO
		_UTL_LOG_W ("pid:[0x%04x] payload_size=[%d]", pid_, payload_size);
		_UTL_LOG_W ("pid:[0x%04x] pointer_field=[%d]", pid_, pointer_field);
		_UTL_LOG_W ("pid:[0x%04x] input data is probably broken", pid_);
		return EN_CHECK_SECTION__INVALID;
	}

	if (pointer_field > 0) {
		_UTL_LOG_D ("pointer_field=[%d]", pointer_field);
		EN_CHECK_SECTION chk = check_section_follow (p, pointer_field);
		switch ((int)chk) {
		case EN_CHECK_SECTION__RECEIVING:
		case EN_CHECK_SECTION__INVALID:
			return EN_CHECK_SECTION__INVALID;
			break;
		}
		p += pointer_field;
		size = size - pointer_field;
	}


	EN_CHECK_SECTION r = EN_CHECK_SECTION__COMPLETED;
	bool is_even_once_completed = false;

	// payload_unit_start_indicator == 1 のパケット内に
	// 複数のセクションが含まれる場合があるので そのためのループ
	while ((size > 0) && (*p != 0xff)) {

//TODO work_sect_info_ check
		if (work_sect_info_) {
			if (work_sect_info_->state_ <= EN_SECTION_STATE__RECEIVING) {
				_UTL_LOG_W ("pid:[0x%04x] unexpected first packet came. start over...", pid_);
				detach_section_list (work_sect_info_);
				work_sect_info_ = NULL;
			}
		}


		section_info *p_attached = attach_section_list (p, size);
		if (!p_attached) {
			return EN_CHECK_SECTION__INVALID;
		}

		// debug dump
		if (arec::common::utils::get_logger()->get_log_level() <= arec::common::logger::level::debug) {
			dump_section_list ();
		}

		// set working addr
		work_sect_info_ = p_attached;

		// debug dump
		if (arec::common::utils::get_logger()->get_log_level() <= arec::common::logger::level::debug) {
			work_sect_info_->dump_header ();
		}

		if (!on_section_started (work_sect_info_)) {
			r = EN_CHECK_SECTION__IGNORE;
			size_t tmp_total_len = SECTION_SHORT_HEADER_LEN + work_sect_info_->get_header ()->section_length;
			if (tmp_total_len >= size) {
				// ignoreしたセクションがパケットをまたいだものだとすると
				// tmp_total_len分はsizeを超えてくるので ループを抜けて次のパケットを見に行きます
				size = 0;
			} else {
				size -= SECTION_SHORT_HEADER_LEN + work_sect_info_->get_header ()->section_length;
				p += SECTION_SHORT_HEADER_LEN + work_sect_info_->get_header ()->section_length;
			}

			detach_section_list (work_sect_info_);
			work_sect_info_ = NULL;

			// is_ignore_section_ 開始します
			is_ignore_section_ = true;

			continue;
		}

		// is_ignore_section_ 解除します
		is_ignore_section_ = false;


		if (work_sect_info_->is_receive_all ()) {
			work_sect_info_->state_ = EN_SECTION_STATE__RECEIVED;

		} else {
			// パケットをまたいだ
			work_sect_info_->state_ = EN_SECTION_STATE__RECEIVING;
			if (is_even_once_completed) {
				return EN_CHECK_SECTION__COMPLETED;
			} else {
				return EN_CHECK_SECTION__RECEIVING;
			}
		}


//TODO
//		if (work_sect_info_->sect_hdr_.section_syntax_indicator != 0) {
//			detach_section_list (work_sect_info_);
//			work_sect_info_ = NULL;
//			return false;
//		}

		uint16_t n_data_part_len = work_sect_info_->get_data_part_len ();
		_UTL_LOG_D ("n_data_part_len %d\n", n_data_part_len);


//		if (type_ == EN_SECTION_TYPE__PSISI) {
			// check CRC
			if (!work_sect_info_->check_crc32 ()) {
				_UTL_LOG_E ("pid:[0x%04x] CRC32 fail", pid_);
				r = EN_CHECK_SECTION__CRC32_ERR;
				size -= SECTION_SHORT_HEADER_LEN + work_sect_info_->get_header ()->section_length;
				p += SECTION_SHORT_HEADER_LEN + work_sect_info_->get_header ()->section_length;

				detach_section_list (work_sect_info_);
				work_sect_info_ = NULL;
				continue;
			}
			_UTL_LOG_D ("CRC32 ok");

//		} else if (type_ == EN_SECTION_TYPE__DSMCC) {
//
//		} else {
//			_UTL_LOG_E ("pid:[0x%04x] EN_SECTION_TYPE is invalid.", pid_);
//		}


		size_t trunclen = work_sect_info_->truncate ();
		truncate (trunclen);

		// debug dump
		if (arec::common::utils::get_logger()->get_log_level() <= arec::common::logger::level::debug) {
			dump_section_list ();
		}

		// すでに持っているsectionかどうかチェック
		section_info *p_found = search_section_list (*work_sect_info_);
		if (p_found && p_found->state_ == EN_SECTION_STATE__COMPLETE) {
			_UTL_LOG_D ("already know section -> detach");
			r = EN_CHECK_SECTION__COMPLETED_ALREADY;
			size -= SECTION_SHORT_HEADER_LEN + work_sect_info_->get_header ()->section_length;
			p += SECTION_SHORT_HEADER_LEN + work_sect_info_->get_header ()->section_length;

			detach_section_list (work_sect_info_);
			work_sect_info_ = NULL;
			continue;

		} else {
			// new section
			_UTL_LOG_D ("new section");
			is_even_once_completed = true;
			r = EN_CHECK_SECTION__COMPLETED;
			size -= SECTION_SHORT_HEADER_LEN + work_sect_info_->get_header ()->section_length;
			p += SECTION_SHORT_HEADER_LEN + work_sect_info_->get_header ()->section_length;

			work_sect_info_->state_ = EN_SECTION_STATE__COMPLETE;
			check_detach_fifo_section_list ();

			on_section_completed (work_sect_info_);

			work_sect_info_ = NULL;
			continue;
		}

	} // while ((size > 0) && (*p != 0xff))

	if (is_even_once_completed) {
		return EN_CHECK_SECTION__COMPLETED;
	} else {
		return r;
	}
}

/**
 *
 */
EN_CHECK_SECTION section_parser::check_section_follow (uint8_t *p_payload, size_t payload_size)
{
	if (is_ignore_section_) {
		return EN_CHECK_SECTION__IGNORE;
	}

	if (!p_payload || (payload_size == 0)) {
		return EN_CHECK_SECTION__INVALID;
	}

	if (!work_sect_info_) {
		_UTL_LOG_W ("pid:[0x%04x] head packet has not arrived yet", pid_);
		return EN_CHECK_SECTION__INVALID;
	}

	if (work_sect_info_->state_ != EN_SECTION_STATE__RECEIVING) {
		_UTL_LOG_W ("pid:[0x%04x] head packet has not arrived yet (not EN_SECTION_STATE__RECEIVING)", pid_);
		return EN_CHECK_SECTION__INVALID;
	}

	uint8_t *p = p_payload;

	section_info *p_attached = attach_section_list (p, payload_size);
	if (!p_attached) {
		return EN_CHECK_SECTION__INVALID;
	}

	if (p_attached != work_sect_info_) {
		_UTL_LOG_E ("BUG: pid:[0x%04x] (p_attached != work_sect_info_)", pid_);
		return EN_CHECK_SECTION__INVALID;
	}

	// debug dump
	if (arec::common::utils::get_logger()->get_log_level() <= arec::common::logger::level::debug) {
		dump_section_list ();
		work_sect_info_->dump_header ();
	}

	if (work_sect_info_->is_receive_all ()) {
		work_sect_info_->state_ = EN_SECTION_STATE__RECEIVED;

	} else {
		// パケットをまたいだ
		work_sect_info_->state_ = EN_SECTION_STATE__RECEIVING;
		return EN_CHECK_SECTION__RECEIVING;
	}


//TODO
//	if (work_sect_info_->sect_hdr_.section_syntax_indicator != 0) {
//		detach_section_list (work_sect_info_);
//		work_sect_info_ = NULL;
//		return false;
//	}

	uint16_t n_data_part_len = work_sect_info_->get_data_part_len ();
	_UTL_LOG_D ("n_data_part_len %d\n", n_data_part_len);


//	if (type_ == EN_SECTION_TYPE__PSISI) {
		// check CRC
		if (!work_sect_info_->check_crc32 ()) {
			_UTL_LOG_E ("pid:[0x%04x] CRC32 fail", pid_);
			detach_section_list (work_sect_info_);
			work_sect_info_ = NULL;
			return EN_CHECK_SECTION__CRC32_ERR;
		}
		_UTL_LOG_D ("CRC32 ok");

//	} else if (type_ == EN_SECTION_TYPE__DSMCC) {
//
//	} else {
//		_UTL_LOG_E ("pid:[0x%04x] EN_SECTION_TYPE is invalid.", pid_);
//	}


	size_t trunclen = work_sect_info_->truncate ();
	truncate (trunclen);

	// debug dump
	if (arec::common::utils::get_logger()->get_log_level() <= arec::common::logger::level::debug) {
		dump_section_list ();
	}

	// すでに持っているsectionかどうかチェック
	section_info *p_found = search_section_list (*work_sect_info_);
	if (p_found && p_found->state_ == EN_SECTION_STATE__COMPLETE) {
		_UTL_LOG_D ("already know section -> detach");
		detach_section_list (work_sect_info_);
		work_sect_info_ = NULL;
		return EN_CHECK_SECTION__COMPLETED_ALREADY;

	} else {
		// new section
		_UTL_LOG_D ("new section");
		work_sect_info_->state_ = EN_SECTION_STATE__COMPLETE;
		check_detach_fifo_section_list ();

		on_section_completed (work_sect_info_);

		return EN_CHECK_SECTION__COMPLETED;
	}
}

/**
 *
 */
EN_CHECK_SECTION section_parser::check_section (const TS_HEADER *p_ts_hdr, uint8_t *p_payload, size_t payload_size)
{
	if (!p_ts_hdr || !p_payload || (payload_size == 0)) {
		return EN_CHECK_SECTION__INVALID;
	}


	pid_ = p_ts_hdr->pid;

	if (p_ts_hdr->payload_unit_start_indicator == 0) {

		_UTL_LOG_D ("check_section_follow");
		return check_section_follow (p_payload, payload_size);

	} else {

//		if (is_async_delete_) {
//			_UTL_LOG_I ("pid:[0x%04x] async_delete -> detach_all_section_list", pid_);
//			detach_all_section_list ();
//			is_async_delete_ = false;
//		}

		_UTL_LOG_D ("check_section_first");
		return check_section_first (p_payload, payload_size);
	}
}

/**
 *
 */
section_info *section_parser::get_latest_complete_section (void) const
{
	section_info *p_tmp = NULL;
	section_info *p_latest = NULL;

	p_tmp = sect_list_top_;
	while (p_tmp) {
		if (p_tmp->state_ == EN_SECTION_STATE__COMPLETE) {
			// update latest section
			p_latest = p_tmp;
		}

		// next set
		p_tmp = p_tmp->next_;
	}

	return p_latest;
}

/**
 * truncate
 */
void section_parser::truncate (size_t truncate_len)
{
	memset (pool_ + pool_ind_ - truncate_len, 0x00, truncate_len);
	pool_ind_ -= truncate_len;
}

/**
 * on_section_started
 */
bool section_parser::on_section_started (const section_info *p_comp_section)
{
	return true;
}

/**
 * on_section_completed
 */
void section_parser::on_section_completed (const section_info *p_comp_section)
{
	return;
}

/**
 *
 */
void section_parser::async_delete (void)
{
	is_async_delete_ = true;
}

/**
 *
 */
void section_parser::check_async_delete (void)
{
	if (is_async_delete_) {
		_UTL_LOG_I ("pid:[0x%04x] async_delete -> detach_all_section_list", pid_);
		detach_all_section_list ();
		is_async_delete_ = false;
	}
}


} // namespace parser
