// *****************************************************************************
    // start include guard
    #ifndef STRINGFUNCTIONS_HPP
    #define STRINGFUNCTIONS_HPP
    
    // include C/C++ headers
    #include <string>           // [ C++ STL ] Strings
    #include <vector>           // [ C++ STL ] Vectors
    #include <cstdint>          // [ ANSI C ] Standard integer types
// *****************************************************************************


// =============================================================================
//      STRING MANIPULATION FUNCTIONS
// =============================================================================


// character-based manipulations
std::string ToLowerCase( const std::string& Text );
std::string ToUpperCase( const std::string& Text );
std::vector<std::string> SplitString( const std::string& str, char separator );

// these replace ALL occurences, in place
void ReplaceCharacter( std::string& Text, char OldChar, char NewChar );
void ReplaceSubstring( std::string& Text, const std::string& OldSubstring, const std::string& NewSubstring );


// =============================================================================
//      TREATMENT OF XML STRINGS
// =============================================================================


std::string XMLBlock( const std::string& BlockName, const std::string& BlockContent );
std::string EscapeXML( const std::string& Unescaped );


// =============================================================================
//      NUMBER FORMATTING FUNCTIONS
// =============================================================================


// number to string conversions in non-decimal formats
std::string Hex( uint32_t Value, int Digits );
std::string Bin( uint32_t Value, int Digits );


// *****************************************************************************
    // end include guard
    #endif
// *****************************************************************************
