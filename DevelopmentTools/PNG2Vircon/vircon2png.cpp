// *****************************************************************************
    // include Vircon common headers
    #include "../../VirconDefinitions/Constants.hpp"
    #include "../../VirconDefinitions/FileFormats.hpp"
    
    // include infrastructure headers
    #include "../DevToolsInfrastructure/Definitions.hpp"
    #include "../DevToolsInfrastructure/FilePaths.hpp"
    #include "../DevToolsInfrastructure/FileSignatures.hpp"
    #include "../DevToolsInfrastructure/StringFunctions.hpp"
    
    // include libpng headers
    #include <png.h>
    
    // include C/C++ headers
    #include <iostream>     // [ C++ STL ] I/O Streams
    #include <string>       // [ C++ STL ] Strings
    #include <stdexcept>    // [ C++ STL ] Exceptions
    
    // declare used namespaces
    using namespace std;
    using namespace V32;
// *****************************************************************************


// =============================================================================
//      GLOBAL VARIABLES
// =============================================================================


bool VerboseMode = false;


// =============================================================================
//      IMAGE TREATMENT
// =============================================================================


int ImageWidth, ImageHeight;
png_bytep *RowPointers = nullptr;

// -----------------------------------------------------------------------------

void LoadVTEX( const char *VTEXFilePath )
{
    // open input file
    FILE *VTEXFile = fopen( VTEXFilePath, "rb" );
    
    if( !VTEXFile )
      throw runtime_error( string("Cannot open intput file \"") + VTEXFilePath + "\"" );
    
    // get size and ensure it is a multiple of 4
    // (otherwise file contents are wrong)
    fseek( VTEXFile, 0, SEEK_END );
    unsigned FileBytes = ftell( VTEXFile );
    
    if( (FileBytes % 4) != 0 )
      throw runtime_error( "Incorrect VTEX file format (file size must be a multiple of 4)" );
    
    // ensure that we can at least load the file header
    if( FileBytes < sizeof(TextureFileFormat::Header) )
      throw runtime_error( "Incorrect VTEX file format (file is too small)" );
    
    // load a texture file signature
    TextureFileFormat::Header VTEXHeader;
    fseek( VTEXFile, 0, SEEK_SET );
    fread( &VTEXHeader, sizeof(TextureFileFormat::Header), 1, VTEXFile );
    
    // check that it is actually a texture file
    if( !CheckSignature( VTEXHeader.Signature, TextureFileFormat::Signature ) )
      throw runtime_error( "Incorrect VTEX file format (file does not have a valid signature)" );
    
    // save image dimensions
    ImageWidth = VTEXHeader.TextureWidth;
    ImageHeight = VTEXHeader.TextureHeight;
    
    // report image size
    if( VerboseMode )
      cout << "VTEX image size is " << ImageWidth << "x" << ImageHeight << endl;
    
    // check texture size limitations
    if( !IsBetween( ImageWidth , 1, Constants::GPUTextureSize )
    ||  !IsBetween( ImageHeight, 1, Constants::GPUTextureSize ) )
      throw runtime_error( "VTEX texture does not have correct dimensions (from 1x1 up to 1024x1024 pixels)" );
    
    // check that file size matches the reported image
    unsigned ExpectedBytes = sizeof(TextureFileFormat::Header) + 4 * ImageWidth * ImageHeight;
    
    if( FileBytes != ExpectedBytes )
      throw runtime_error( "Incorrect VTEX file format (file size does not match reported image dimensions)" );
    
    // now read every row
    RowPointers = (png_bytep*)malloc(sizeof(png_bytep) * ImageHeight);
    
    for( int y = 0; y < ImageHeight; y++ )
    {
        RowPointers[y] = (png_byte*)malloc( ImageWidth*4 );
        fread( RowPointers[ y ], ImageWidth*4, 1, VTEXFile );
    }
    
    // clean-up
    fclose( VTEXFile );
}

// -----------------------------------------------------------------------------

void SavePNG( const char *PNGFilePath )
{
    // open output file
    FILE *PNGFile = fopen( PNGFilePath, "wb" );
    
    if( !PNGFile )
      throw runtime_error( "Cannot open output file for writing" );

    png_structp PNGHandler = png_create_write_struct( PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr );
    
    if( !PNGHandler )
      throw runtime_error( "Cannot create a PNG handler" );

    png_infop PNGInfo = png_create_info_struct( PNGHandler );
    
    if( !PNGInfo )
      throw runtime_error( "Cannot create a PNG info structure" );

    // define a callback function expected by libpng for error handling
    if( setjmp( png_jmpbuf(PNGHandler) ) )
      throw runtime_error( "Cannot set callback function for libpng" );
      
    // begin writing
    if( VerboseMode )
      cout << "Starting PNG I/O to write to file" << endl;
    
    png_init_io( PNGHandler, PNGFile );

    // define output as 8bit depth in RGBA format
    png_set_IHDR
    (
        PNGHandler,
        PNGInfo,
        ImageWidth, ImageHeight,
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    
    // write basic image info
    png_write_info( PNGHandler, PNGInfo );
    
    // write the actual pixel data for all rows
    if( !RowPointers )
      throw runtime_error( "The row pointers are not allocated" );
    
    png_write_image( PNGHandler, RowPointers );
    
    // end writing
    png_write_end( PNGHandler, nullptr );
    
    // clean-up
    fclose( PNGFile );
    png_destroy_write_struct( &PNGHandler, &PNGInfo );
}


// =============================================================================
//      AUXILIARY FUNCTIONS
// =============================================================================


void PrintUsage()
{
    cout << "USAGE: vircon2png [options] file" << endl;
    cout << "Options:" << endl;
    cout << "  --help       Displays this information" << endl;
    cout << "  --version    Displays program version" << endl;
    cout << "  -o <file>    Output file, default name is the same as input" << endl;
    cout << "  -v           Displays additional information (verbose)" << endl;
}

// -----------------------------------------------------------------------------

void PrintVersion()
{
    cout << "vircon2png v24.8.2" << endl;
    cout << "Vircon32 PNG file extractor by Javier Carracedo" << endl;
}


// =============================================================================
//      MAIN FUNCTION
// =============================================================================


int main( int NumberOfArguments, char* Arguments[] )
{
    try
    {
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Process command line arguments
        
        // variables to capture input parameters
        string InputPath, OutputPath;
        
        // process arguments
        for( int i = 1; i < NumberOfArguments; i++ )
        {
            if( Arguments[i] == string("--help") )
            {
                PrintUsage();
                return 0;
            }
            
            if( Arguments[i] == string("--version") )
            {
                PrintVersion();
                return 0;
            }
            
            if( Arguments[i] == string("-v") )
            {
                VerboseMode = true;
                continue;
            }
            
            if( Arguments[i] == string("-o") )
            {
                // expect another argument
                i++;
                
                if( i >= NumberOfArguments )
                  throw runtime_error( "missing filename after '-o'" );
                
                // now we can safely read the input path
                OutputPath = Arguments[ i ];
                continue;
            }
            
            // discard any other parameters starting with '-'
            if( Arguments[i][0] == '-' )
              throw runtime_error( string("unrecognized command line option '") + Arguments[i] + "'" );
            
            // any non-option parameter is taken as the input file
            if( InputPath.empty() )
            {
                InputPath = Arguments[i];
            }
            
            // only a single input file is supported!
            else
              throw runtime_error( "too many input files, only 1 is supported" );
        }
        
        // check if an input path was given
        if( InputPath.empty() )
          throw runtime_error( "no input file" );
        
        // if output path was not given, just
        // replace the extension in the input
        if( OutputPath.empty() )
        {
            OutputPath = ReplaceFileExtension( InputPath, "png" );
            
            if( VerboseMode )
              cout << "using output path: \"" << OutputPath << "\"" << endl;
        }
        
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // STEP 1: Load the VTEX file
        
        if( VerboseMode )
          cout << "loading input file \"" << InputPath << "\"" << endl;
        
        LoadVTEX( InputPath.c_str() );
        
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // STEP 2: Save the PNG image
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        
        if( VerboseMode )
          cout << "saving output file \"" << OutputPath << "\"" << endl;
        
        SavePNG( OutputPath.c_str() );
    }
    
    catch( const exception& e )
    {
        cerr << "vircon2png: error: " << e.what() << endl;
        return 1;
    }
    
    // perform clean-up
    for( int y = 0; y < ImageHeight; y++ )
      free( RowPointers[ y ] );
    
    free( RowPointers );
    
    // report success
    if( VerboseMode )
      cout << "conversion successful" << endl;
    
    return 0;
}
