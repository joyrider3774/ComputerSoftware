// *****************************************************************************
    // include common Vircon headers
    #include "../../VirconDefinitions/Constants.hpp"
    #include "../../VirconDefinitions/FileFormats.hpp"
    
    // include infrastructure headers
    #include "../DesktopInfrastructure/FilePaths.hpp"
    #include "../DesktopInfrastructure/FileSignatures.hpp"
    #include "../DesktopInfrastructure/LogStream.hpp"
    
    // include project headers
    #include "V32MemoryCardController.hpp"
    
    // include C/C++ headers
    #include <iostream>     // [ C++ STL ] I/O Streams
    #include <fstream>      // [ C++ STL ] File streams
    
    // declare used namespaces
    using namespace std;
// *****************************************************************************


namespace V32
{
    // =============================================================================
    //      V32 MEMORY CARD CONTROLLER: OWN METHODS
    // =============================================================================
    
    
    V32MemoryCardController::V32MemoryCardController()
    {
        PendingSave = false;
    }
    
    // -----------------------------------------------------------------------------
    
    bool V32MemoryCardController::ReadPort( int32_t LocalPort, VirconWord& Result )
    {
        // check range
        if( LocalPort > MEM_LastPort )
          return false;
        
        // provide value
        if( LocalPort == (int32_t)MEM_LocalPorts::Connected )
          Result.AsInteger = (MemorySize > 0? 1 : 0);
        
        return true;
    }
    
    // -----------------------------------------------------------------------------
    
    bool V32MemoryCardController::WritePort( int32_t LocalPort, VirconWord Value )
    {
        // all memory card ports are read-only!
        return false;
    }
    
    // -----------------------------------------------------------------------------
    
    void V32MemoryCardController::ChangeFrame()
    {
        // save to file when needed
        if( !PendingSave ) return;
        
        SaveContents( CardSavePath );
        PendingSave = false;
    }
    
    
    // =============================================================================
    //      V32 MEMORY CARD CONTROLLER: METHODS OVERRIDEN FROM RAM
    // =============================================================================
    
    
    bool V32MemoryCardController::WriteAddress( int32_t LocalAddress, VirconWord Value )
    {
        // check that the normal RAM write is successful
        if( !V32RAM::WriteAddress( LocalAddress, Value ) )
          return false;
        
        // data is now pending to save
        PendingSave = true;
        
        return true;
    }
    
    // -----------------------------------------------------------------------------
    
    void V32MemoryCardController::LoadContents( const std::string& FilePath )
    {
        // open the file
        LOG( "Loading memory card file \"" << FilePath << "\"" );
        
        ifstream InputFile;
        InputFile.open( FilePath, ios::binary | ios::ate );
        
        if( InputFile.fail() )
          THROW( "Cannot open memory card file" );
        
        // check file size coherency
        int NumberOfBytes = InputFile.tellg();
        int ExpectedBytes = 8 + Constants::MemoryCardSize * 4;
        
        if( NumberOfBytes != ExpectedBytes )
        {
            InputFile.close();
            THROW( "Invalid memory card: File does not match the size of a Vircon memory card" );
        }
        
        // read and check signature
        InputFile.seekg( 0, ios_base::beg );
        char FileSignature[ 8 ];
        InputFile.read( FileSignature, 8 );
        
        if( !CheckSignature( FileSignature, Signatures::MemoryCardFile ) )
          THROW( "Memory card file does not have a valid signature" );
        
        // connect the memory
        Connect( Constants::MemoryCardSize );
        
        // now load the whole memory card contents
        InputFile.read( (char*)(&Memory[0]), MemorySize * 4 );
        
        // close the file
        InputFile.close();
        
        // save the file path for later
        CardSavePath = FilePath;
        CardFileName = GetPathFileName( FilePath );
    }
    
    // -----------------------------------------------------------------------------
    
    void V32MemoryCardController::SaveContents( const std::string& FilePath )
    {
        // open the file
        LOG( "Saving memory card file \"" << FilePath << "\"" );
        
        ofstream OutputFile;
        OutputFile.open( FilePath, ios::binary );
        
        if( OutputFile.fail() )
          THROW( "Cannot open memory card file" );
        
        // save the signature
        WriteSignature( OutputFile, Signatures::MemoryCardFile );
        
        // now save all contents
        OutputFile.write( (char*)(&Memory[0]), MemorySize * 4 );
        
        // close the file
        OutputFile.close();
    }
    
    // -----------------------------------------------------------------------------
    
    void V32MemoryCardController::CreateNewFile( const std::string& FilePath )
    {
        LOG( "Creating memory card file \"" << FilePath << "\"" );
        
        // open the file
        ofstream OutputFile;
        OutputFile.open( FilePath, ios::binary | ios::trunc );
        
        if( OutputFile.fail() )
          THROW( "Cannot create memory card file" );
        
        // save the signature
        WriteSignature( OutputFile, Signatures::MemoryCardFile );
        
        // now save all empty contents
        vector< VirconWord > EmptyWords;
        EmptyWords.resize( Constants::MemoryCardSize );
        OutputFile.write( (char*)(&EmptyWords[0]), Constants::MemoryCardSize * 4 );
        
        // close the file
        OutputFile.close();
    }
}
