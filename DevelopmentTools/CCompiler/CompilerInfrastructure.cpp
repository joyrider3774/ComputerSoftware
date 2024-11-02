// *****************************************************************************
    // include common Vircon headers
    #include "../../VirconDefinitions/Constants.hpp"
    
    // include project headers
    #include "CTokens.hpp"
    #include "Globals.hpp"
    #include "CompilerInfrastructure.hpp"
    
    // include C/C++ headers
    #include <iostream>     // [ C++ STL ] I/O Streams
    #include <fstream>      // [ C++ STL ] File streams
    #include <map>          // [ C++ STL ] Maps
    #include <algorithm>    // [ C++ STL ] Algorithms
    
    // declare used namespaces
    using namespace std;
// *****************************************************************************


// =============================================================================
//      SPECIFIC STRING MANIPULATIONS
// =============================================================================


string XMLBlock( const string& BlockName, const string& BlockContent )
{
    return "<" + BlockName + ">" + BlockContent + "</" + BlockName + ">";
}

// -----------------------------------------------------------------------------

string EscapeXML( const string& Unescaped )
{
    string Escaped = Unescaped;
    
    // replace ampersand before anything else,
    // since other escape sequences will add
    // extra ampersands that we shouldn't escape
    ReplaceSubstring( Escaped, "&", "&amp;" );
    ReplaceSubstring( Escaped, "<", "&lt;" );
    ReplaceSubstring( Escaped, ">", "&gt;" );
    ReplaceSubstring( Escaped, "'", "&apos;" );
    ReplaceSubstring( Escaped, "\"", "&quot;" );
    return Escaped;
}

// -----------------------------------------------------------------------------

// this replaces ALL occurences, in place
void ReplaceCharacter( string& Text, char OldChar, char NewChar )
{
    replace( Text.begin(), Text.end(), OldChar, NewChar );
}

// -----------------------------------------------------------------------------

// this replaces ALL occurences, in place
void ReplaceSubstring( string& Text, const string& OldSubstring, const string& NewSubstring )
{
    size_t Position = 0;
    
    while( (Position = Text.find( OldSubstring, Position )) != string::npos )
    {
        Text.replace( Position, OldSubstring.length(), NewSubstring );
        Position += NewSubstring.length();
    }
}

// -----------------------------------------------------------------------------

string EscapeCCharacter( char c )
{
    // in our tools, non standard ASCII characters
    // should always be expressed numerically
    if( c & 0x80 )
    {
        static string HexChars = "0123456789ABCDEF";
        unsigned char uc = *((unsigned char*)&c);
        string Escaped = "\\x";
        Escaped += HexChars[ uc >> 4 ];
        Escaped += HexChars[ uc & 0xF ];
        return Escaped;
    }
    
    // the null character will always need to
    // be escaped, or the string may be incorrect
    if( !c ) return "\\x00";
    
    // our supported escape sequences
    switch( c )
    {
        case '\\': return "\\\\";
        case '\"': return "\\\"";
        case '\'': return "\\'";
        case '\n': return "\\n";
        case '\r': return "\\r";
        case '\t': return "\\t";
        default: return string{ c };
    }
}

// -----------------------------------------------------------------------------

// produces a new string, instead of modifying the original
string EscapeCString( const string& Text )
{
    string Escaped;
    
    for( char c : Text )
      Escaped += EscapeCCharacter( c );
    
    return Escaped;
}


// =============================================================================
//      ERROR HANDLING
// =============================================================================


// global flag to abort when there were errors
int CompilationErrors;
int CompilationWarnings;

// maximums to report
#define MAXIMUM_ERRORS   15
#define MAXIMUM_WARNINGS 10

// -----------------------------------------------------------------------------

void RaiseWarning( SourceLocation Location, const std::string& Description )
{
    CompilationWarnings++;
    
    // ignore warning when needed
    if( DisableWarnings || CompilationWarnings > MAXIMUM_WARNINGS )
      return;
    
    // warn if no further warnings will be reported
    if( CompilationWarnings == MAXIMUM_WARNINGS )
    {
        cerr << "warning: maximum warnings have been reached" << endl;
        return;
    }
    
    // otherwise report the warning normally
    cerr << Location.FilePath << ':' << Location.Line << ':' << Location.Column;
    cerr << ": warning: " << Description << endl;
}

// -----------------------------------------------------------------------------

void RaiseError( SourceLocation Location, const std::string& Description )
{
    CompilationErrors++;
    
    // stop compilation after maximum errors
    if( CompilationErrors >= MAXIMUM_ERRORS )
      throw runtime_error( "error: maximum errors have been reached" );
    
    // otherwise report the error normally
    cerr << Location.FilePath << ':' << Location.Line << ':' << Location.Column;
    cerr << ": error: " << Description << endl;
}

// -----------------------------------------------------------------------------

void RaiseFatalError( SourceLocation Location, const std::string& Description )
{
    // report the fatal error
    cerr << Location.FilePath << ':' << Location.Line << ':' << Location.Column;
    cerr << ": fatal error: " << Description << endl;
    
    // stop compilation
    throw runtime_error( "compilation terminated" );
}


// =============================================================================
//      SUPPORT FUNCTIONS FOR TOKENS
// =============================================================================


void ExpectSameLine( CToken* Start, CToken* Current )
{
    if( !AreInSameLine( Start, Current ) )
      RaiseFatalError( Start->Location, "unexpected end of line" );
}

// -----------------------------------------------------------------------------

void ExpectEndOfLine( CToken* Start, CToken* Current )
{
    if( AreInSameLine( Start, Current ) )
      RaiseFatalError( Current->Location, "uxpected end of line" );
}

// -----------------------------------------------------------------------------

void ExpectSpecialSymbol( CTokenIterator& TokenPosition, SpecialSymbolTypes Expected )
{
    CToken* NextToken = *TokenPosition;
    
    // first check for end of file
    if( IsLastToken( NextToken ) )
    {
        SourceLocation Location = (*Previous(TokenPosition))->Location;
        RaiseFatalError( Location, "unexpected end of file" );
    }
    
    // expected case
    if( NextToken->Type() == CTokenTypes::SpecialSymbol )
    {
        SpecialSymbolToken* NextSymbol = (SpecialSymbolToken*)NextToken;
        
        if( NextSymbol->Which == Expected )
        {
            // consume the symbol and exit
            TokenPosition++;
            return;
        }
    }
    
    // other unexpected cases
    SourceLocation Location = NextToken->Location;
    RaiseFatalError( Location, "expected " + SpecialSymbolToString( Expected ) );
}

// -----------------------------------------------------------------------------

void ExpectDelimiter( CTokenIterator& TokenPosition, DelimiterTypes Expected )
{
    CToken* NextToken = *TokenPosition;
    
    // first check for end of file
    if( IsLastToken( NextToken ) )
    {
        SourceLocation Location = (*Previous(TokenPosition))->Location;
        RaiseFatalError( Location, "unexpected end of file" );
    }
    
    // expected case
    if( NextToken->Type() == CTokenTypes::Delimiter )
    {
        DelimiterToken* NextDelimiter = (DelimiterToken*)NextToken;
        
        if( NextDelimiter->Which == Expected )
        {
            // consume the delimiter and exit
            TokenPosition++;
            return;
        }
    }
    
    // other unexpected cases
    SourceLocation Location = NextToken->Location;
    RaiseFatalError( Location, "expected " + DelimiterToString( Expected ) );
}

// -----------------------------------------------------------------------------

void ExpectKeyword( CTokenIterator& TokenPosition, KeywordTypes Expected )
{
    CToken* NextToken = *TokenPosition;
    
    // first check for end of file
    if( IsLastToken( NextToken ) )
    {
        SourceLocation Location = (*Previous(TokenPosition))->Location;
        RaiseFatalError( Location, "unexpected end of file" );
    }
    
    // expected case
    if( NextToken->Type() == CTokenTypes::Keyword )
    {
        KeywordToken* NextKeyword = (KeywordToken*)NextToken;
        
        if( NextKeyword->Which == Expected )
        {
            // consume the keyword and exit
            TokenPosition++;
            return;
        }
    }
    
    // other unexpected cases
    SourceLocation Location = NextToken->Location;
    RaiseFatalError( Location, "expected " + KeywordToString( Expected ) );
}

// -----------------------------------------------------------------------------

void ExpectOperator( CTokenIterator& TokenPosition, OperatorTypes Expected )
{
    CToken* NextToken = *TokenPosition;
    
    // first check for end of file
    if( IsLastToken( NextToken ) )
    {
        SourceLocation Location = (*Previous(TokenPosition))->Location;
        RaiseFatalError( Location, "unexpected end of file" );
    }
    
    // expected case
    if( NextToken->Type() == CTokenTypes::Operator )
    {
        OperatorToken* NextOperator = (OperatorToken*)NextToken;
        
        if( NextOperator->Which == Expected )
        {
            // consume the operator and exit
            TokenPosition++;
            return;
        }
    }
    
    // other unexpected cases
    SourceLocation Location = NextToken->Location;
    RaiseFatalError( Location, "expected " + OperatorToString( Expected ) );
}

// -----------------------------------------------------------------------------

string ExpectIdentifier( CTokenIterator& TokenPosition )
{
    CToken* NextToken = *TokenPosition;
    
    // first check for end of file
    if( IsLastToken( NextToken ) )
    {
        SourceLocation Location = (*Previous(TokenPosition))->Location;
        RaiseFatalError( Location, "unexpected end of file" );
    }
    
    // expected case
    if( NextToken->Type() == CTokenTypes::Identifier )
    {
        IdentifierToken* NextIdentifier = (IdentifierToken*)NextToken;
        
        // consume the identifier
        TokenPosition++;
        
        // provide the name
        return NextIdentifier->Name;
    }
    
    // other unexpected cases
    SourceLocation Location = NextToken->Location;
    RaiseFatalError( Location, "expected identifier" );
    
    // avoid compiler warning
    return "";
}
