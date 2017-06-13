/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Baranyi, Botond
 *
 ******************************************************************************/

#ifndef JSON_TOKENIZER_HH
#define	JSON_TOKENIZER_HH

#include <cstddef>

/** JSON token types */
enum json_token_t {
  JSON_TOKEN_ERROR = 0,     // not actually a token, used when get_next_token() fails
  JSON_TOKEN_NONE,          // not actually a token, used for initializing
  JSON_TOKEN_OBJECT_START,  // "{"
  JSON_TOKEN_OBJECT_END,    // "}"
  JSON_TOKEN_ARRAY_START,   // "["
  JSON_TOKEN_ARRAY_END,     // "]"
  JSON_TOKEN_NAME,          // field name (key) in a JSON object, followed by ":"
  JSON_TOKEN_NUMBER,        // JSON number value
  JSON_TOKEN_STRING,        // JSON string value
  JSON_TOKEN_LITERAL_TRUE,  // "true" value
  JSON_TOKEN_LITERAL_FALSE, // "false" value
  JSON_TOKEN_LITERAL_NULL   // "null" value
};
  
/** A class for building and processing JSON documents. Stores the document in a buffer.
  * Can build JSON documents by inserting tokens into an empty buffer.
  * Can extract tokens from an existing JSON document. */
class JSON_Tokenizer {
  
private:
  
  /** The buffer that stores the JSON document 
    * This is a buffer with exponential allocation (expstring), only uses expstring
    * memory operations from memory.h (ex.: mputstr, mputprintf) */
  char* buf_ptr;
  
  /** Number of bytes currently in the buffer */
  size_t buf_len;
  
  /** Current position in the buffer */
  size_t buf_pos;
  
  /** Current depth in the JSON document (only used if pretty printing is set */
  unsigned int depth;
  
  /** Stores the previous JSON token inserted by put_next_token() */
  json_token_t previous_token;
  
  /** Activates or deactivates pretty printing
    * If active, put_next_token() and put_separator() will add extra newlines 
    * and indenting to the JSON code to make it more readable for you humans,
    * otherwise it will be compact (no white spaces). */
  bool pretty;
  
  /** Initializes the properties of the tokenizer. 
    * The buffer is initialized with the parameter data (unless it's empty). */
  void init(const char* p_buf, const size_t p_buf_len);
  
  /** Inserts a character to the end of the buffer */
  void put_c(const char c);
  
  /** Inserts a null-terminated string to the end of the buffer */
  void put_s(const char* s);
  
  /** Indents a new line in JSON code depending on the current depth.
    * If the maximum depth is reached, the code is not indented further.
    * Used only if pretty printing is set. */
  void put_depth();
  
  /** Skips white spaces until a non-white-space character is found.
    * Returns false if the end of the buffer is reached before a non-white-space
    * character is found, otherwise returns true. */
  bool skip_white_spaces();
  
  /** Attempts to find a JSON string at the current buffer position. 
    * Returns true if a valid string is found before the end of the buffer
    * is reached, otherwise returns false. */
  bool check_for_string();
  
  /** Attempts to find a JSON number at the current buffer position.
    * For number format see http://json.org/.
    * Returns true if a valid number is found before the end of the buffer
    * is reached, otherwise returns false. */
  bool check_for_number();
  
  /** Checks if the current character in the buffer is a valid JSON separator.
    * Separators are: commas (,), colons (:) and curly and square brackets ({}[]).
    * This function also steps over the separator if it's a comma.
    * Returns true if a separator is found, otherwise returns false. */
  bool check_for_separator();
  
  /** Attempts to find a specific JSON literal at the current buffer position.
    * Returns true if the literal is found, otherwise returns false.
    * @param p_literal [in] Literal value to find */
  bool check_for_literal(const char* p_literal);
  
  /** Adds a separating comma (,) if the previous token is a value, or an object or
    * array end mark. */
  void put_separator();
  
  /** No copy constructor. Implement if needed. */
  JSON_Tokenizer(const JSON_Tokenizer&);
  
  /** No assignment operator. Implement if needed. */
  JSON_Tokenizer& operator=(const JSON_Tokenizer&);
  
public:
  /** Constructs a tokenizer with an empty buffer.
    * Use put_next_token() to build a JSON document and get_buffer()/get_buffer_length() to retrieve it */
  JSON_Tokenizer(bool p_pretty = false) : pretty(p_pretty) { init(0, 0); }
  
  /** Constructs a tokenizer with the buffer parameter.
    * Use get_next_token() to read JSON tokens and get_pos()/set_pos() to move around in the buffer */
  JSON_Tokenizer(const char* p_buf, const size_t p_buf_len) : pretty(false) { init(p_buf, p_buf_len); }
  
  /** Destructor. Frees the buffer. */
  ~JSON_Tokenizer();
  
  /** Reinitializes the tokenizer with a new buffer. */
  inline void set_buffer(const char* p_buf, const size_t p_buf_len) { init(p_buf, p_buf_len); }
  
  /** Retrieves the buffer containing the JSON document. */
  inline const char* get_buffer() { return buf_ptr; }
  
  /** Retrieves the length of the buffer containing the JSON document. */
  inline size_t get_buffer_length() { return buf_len; }
  
  /** Extracts a JSON token from the current buffer position.
    * @param p_token [out] Extracted token type, or JSON_TOKEN_ERROR if no token
    * could be extracted, or JSON_TOKEN_NONE if the buffer end is reached
    * @param p_token_str [out] A pointer to the token data (if any):
    * the name of a JSON object field (without quotes), or the string representation
    * of a JSON number, or a JSON string (with quotes and double-escaped).
    * @param p_str_len [out] The character length of the token data (if there is data)
    * @return The number of characters extracted 
    * @note The token data is not copied, *p_token_str will point to the start of the 
    * data in the tokenizer's buffer. */
  size_t get_next_token(json_token_t* p_token, char** p_token_str, size_t* p_str_len);
  
  /** Gets the current read position in the buffer.
    * This is where get_next_token() will read from next. */
  inline size_t get_buf_pos() { return buf_pos; }
  
  /** Sets the current read position in the buffer.
    * This is where get_next_buffer() will read from next. */
  inline void set_buf_pos(const size_t p_buf_pos) { buf_pos = p_buf_pos; }
  
  /** Adds the specified JSON token to end of the buffer. 
    * @param p_token [in] Token type
    * @param p_token_str [in] The name of a JSON object field (without quotes), or
    * the string representation of a JSON number, or a JSON string (with quotes 
    * and double-escaped). For all the other tokens this parameter will be ignored.
    * @return The number of characters added to the JSON document */
  int put_next_token(json_token_t p_token, const char* p_token_str = 0);
  
  /** Adds raw data to the end of the buffer.
    * @param p_data [in] Pointer to the beginning of the data
    * @param p_len [in] Length of the data in bytes */
  void put_raw_data(const char* p_data, size_t p_len);
  
}; // class JSON_Tokenizer

// A dummy JSON tokenizer, use when there is no actual JSON document
static JSON_Tokenizer DUMMY_BUFFER;

/** Converts a string into a JSON string by replacing all control characters
  * with JSON escape sequences, if available, or with the \uHHHH escape sequence.
  * The string is also wrapped inside a set of double quotes and all double quotes
  * and backslash characters are double-escaped.
  *
  * Returns an expstring, that needs to be freed. */
extern char* convert_to_json_string(const char* str);


#endif	/* JSON_TOKENIZER_HH */

