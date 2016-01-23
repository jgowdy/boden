#ifndef BDN_Utf16Codec_H_
#define BDN_Utf16Codec_H_

#include <string>

namespace bdn
{

/** The Utf-16 string codec.

	The template parameter EL must be a 16 bit integer type. Usually either char16_t or
	wchar_t (on systems where wchar_t is 16 bit)
*/
template<class EL>
class Utf16Codec
{
public:

	/** The encoded element type. This is a 16 bit integer type.*/
	typedef EL EncodedElement;


	/** The std string type for the encoded string.*/
	typedef std::basic_string<EncodedElement> EncodedString;


	/** A character iterator that decodes UTF-16 data from an
		arbitrary source iterator into Unicode characters (char32_t).
	*/
	template<class SourceIterator>
	class DecodingIterator : public std::iterator<std::bidirectional_iterator_tag, char32_t>
	{
	public:
		/** @param sourceIt the source iterator that provides the UTF-16 data.
			@param beginSourceIt an iterator that points to the beginning of the valid source data.
				The implementation uses this to avoid overshooting the data boundaries if the UTF-16 data is corrupted.
				This is often the same as sourceIt.
			@param endSourceIt an iterator that points to the end of the valid source data
				(the position after the last valid source element).
				The implementation uses this to avoid overshooting the data boundaries if the UTF-16 data is corrupted.
		*/
		DecodingIterator(const SourceIterator& sourceIt, const SourceIterator& beginSourceIt, const SourceIterator& endSourceIt)
		{
			_sourceIt = sourceIt;
			_beginSourceIt = beginSourceIt;
			_endSourceIt = endSourceIt;

			_chr = (char32_t)-1;
		}

		DecodingIterator()
		{
			_chr = (char32_t)-1;
		}


		DecodingIterator& operator++()
		{
			if(_chr==(char32_t)-1)
			{
				// we still need to decode the current character (the
				// one at the position we are at right now). We need to
				// do that so that _nextIt will be valid.
				decode();
			}

			_sourceIt = _nextIt;

			_chr=(char32_t)-1;

			return *this;
		}

		DecodingIterator operator++(int)
		{
			DecodingIterator oldVal = *this;

			operator++();

			return oldVal;
		}

		DecodingIterator& operator--()
		{
			_nextIt = _sourceIt;

			// iterate backwards until we find the beginning of
			// a sequence.
			_sourceIt--;
			_chr = *_sourceIt;
			if(_chr>=0xd800 && _chr<=0xdfff)
			{
				if(_chr<0xdc00)
				{
					// a leading surrogate at the end of a sequence => invalid
					_chr = 0xfffd;
				}
				else
				{
					// a trailing surrogate. Need one more value.
					if(_sourceIt==_beginSourceIt)
					{
						// invalid sequence at the start. Cannot go further back.
						_chr = 0xfffd;
					}
					else
					{
						_sourceIt--;
						char32_t highVal = *_sourceIt;
						if(highVal<0xd800 || highVal>0xdbff)
						{
							// invalid leading surrogate. Which means that the
							// first one is also invalid.
							_chr = 0xfffd;
							++_sourceIt;
						}
						else
						{
							highVal -= 0xd800;

							_chr -= 0xdc00;
							_chr |= highVal<<10;
							_chr += 0x010000;
						}
					}
				}
			}

			return *this;
		}

		DecodingIterator operator--(int)
		{
			DecodingIterator oldVal = *this;

			operator--();

			return oldVal;
		}

		char32_t operator*()
		{
			if(_chr==(char32_t)-1)
				decode();

			return _chr;
		}

		bool operator==(const DecodingIterator& o) const
		{
			return (_sourceIt==o._sourceIt);
		}

		bool operator!=(const DecodingIterator& o) const
		{
			return !operator==(o);
		}


	protected:
		void decode()
		{
			_nextIt = _sourceIt;

			_chr = *_nextIt;
			++_nextIt;

			if(_chr>=0xd800 && _chr<=0xdfff)
			{
				if(_chr>=0xdc00)
				{
					// trailing surrogate at the beginning => invalid
					_chr = 0xfffd;
				}
				else
				{					
					// leading surrogate

					if(_nextIt==_endSourceIt)
					{
						// no more data after the initial surrogate. Invalid value.
						_chr = 0xfffd;
					}
					else
					{
						char32_t lowVal = *_nextIt;					
						if(lowVal>=0xdc00 && lowVal<=0xdfff)
						{
							// valid trailing surrogate
							lowVal -= 0xdc00;
							_chr-=0xd800;

							_chr<<=10;
							_chr |= lowVal;

							_chr += 0x010000;

							++_nextIt;
						}
						else
						{
							// not a valid trailing surrogate. So the leading surrogate should
							// be treated as an invalid value.
							_chr = 0xfffd;
						}
					}
				}
			}
		}

		SourceIterator  _sourceIt;
		SourceIterator  _beginSourceIt;
		SourceIterator  _endSourceIt;

		char32_t         _chr;
		SourceIterator  _nextIt;
	};



	/** A decoding iterator that works on the iterator type of the encoded standard string type
		#EncodedString.*/
	typedef DecodingIterator<typename EncodedString::const_iterator> DecodingStringIterator;



	/** Encodes unicode characters to UTF-16.*/
	template<class SourceIterator>
	class EncodingIterator : public std::iterator<std::bidirectional_iterator_tag, EncodedElement>
	{
	public:
		EncodingIterator(const SourceIterator& sourceIt)
		{
			_sourceIt = sourceIt;

			// we cannot call encode yet, because sourceIt might be
			// at an invalid position (i.e. end()). So instead we use an
			// invalid offset and encode the first time we dereference.
			_offset=-1;
		}

		EncodingIterator()
		{
			_offset = -1;
		}


		EncodingIterator& operator++()
		{
			if(_offset==-1)
				encode();

			++_offset;
			if(_offset>=_encodedLength)
			{
				++_sourceIt;
				_offset=-1;
			}

			return *this;
		}

		EncodingIterator operator++(int)
		{
			EncodingIterator oldVal = *this;

			operator++();

			return oldVal;
		}

		EncodingIterator& operator--()
		{
			if(_offset<=0)
			{
				// _offset==-1 means that the current character has not yet been encoded. That also
				// means that we are at the beginning of the current character.

				_sourceIt--;
				encode();

				// move to the last value of the sequence.
				_offset = _encodedLength-1;
			}
			else
				_offset--;

			return *this;
		}

		EncodingIterator operator--(int)
		{
			EncodingIterator oldVal = *this;

			operator--();

			return oldVal;
		}

		EncodedElement operator*()
		{
			if(_offset==-1)
				encode();

			return (EncodedElement)_encoded[_offset];
		}

		bool operator==(const EncodingIterator<SourceIterator>& o) const
		{
			if(_sourceIt!=o._sourceIt)
				return false;

			// 0 and -1 are the same position (-1 means "0 but still need to call encode")
			return (_offset==o._offset || (_offset<=0 && o._offset<=0));
		}

		bool operator!=(const EncodingIterator<SourceIterator>& o) const
		{
			return !operator==(o);
		}


	protected:
		void encode()
		{
			char32_t chr = *_sourceIt;

			if(chr<0 || chr>0x10ffff || (chr>=0xd800 && chr<=0xdfff))
			{
				// invalid, unencodable or in the surrogate range. Use 'replacement character'
				chr = 0xfffd;
			}

			if(chr>=0x10000)
			{
				// encode as surrogate pair
				chr -= 0x10000;
				_encoded[0] = (chr>>10) + 0xd800;
				_encoded[1] = (chr & 0x03ff) + 0xdc00;
				_encodedLength = 2;
			}
			else
			{
				_encoded[0] = chr;
				_encodedLength = 1;
			}

			_offset = 0;
		}


		SourceIterator  _sourceIt;
		int             _encoded[2];
		int             _encodedLength;
		int             _offset;
	};




};


}

#endif