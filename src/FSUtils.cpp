#include "PrintDebugTools.h"
#include <OS.h>
#include <Volume.h>
#include <Directory.h>
#include <File.h>
#include <Entry.h>
#include <Path.h>
#include <Alert.h>
#include <stdio.h>
#include <stdlib.h>
#include <fs_attr.h>
#include <String.h>
#include <string.h>
#include <Errors.h>
#include "FSUtils.h"

#define COPY_BUFFER_SIZE 8192

status_t CheckCopiable(BEntry *src,BEntry *dest)
{
	// Checks to see if we can copy the src to dest.
	if(!src || !dest)
		return B_ERROR;
	
	if(!src->Exists())
		return B_ENTRY_NOT_FOUND;

	// Ensure that when we want the destination directory, that is exactly
	// what we're working with. If we've been given an entry which is a file,
	// extract the destination directory.
	BEntry destdir;		
	if(dest->IsDirectory())
		destdir=*dest;
	else
		dest->GetParent(&destdir);
	
	// check existence of target directory
	if(!destdir.Exists())
		return B_NAME_NOT_FOUND;
		
	// check space
	entry_ref ref;
	off_t src_bytes;


	dest->GetRef(&ref);
	BVolume dvolume(ref.device);
	src->GetSize(&src_bytes);

	if(src_bytes>dvolume.FreeBytes())
		return B_DEVICE_FULL;
	
	// check permissions
	if(dvolume.IsReadOnly())
		return B_READ_ONLY;

	// check existing name
	BPath path;
	destdir.GetPath(&path);
	

	char name[B_FILE_NAME_LENGTH];
	src->GetName(name);
	
	BString newpath=path.Path();
	newpath+=name;

	BFile file;
	if(file.SetTo(newpath.String(),B_READ_WRITE | B_FAIL_IF_EXISTS)==B_FILE_EXISTS)
	{
		// We have an existing file, so query the user what to do.
		status_t returncode;

		newpath="The file ";
		newpath+=name;
		newpath+=" exists. Do you want to replace it?";
		
		BAlert *alert=new BAlert("Error", newpath.String(), "Replace File", "Skip File", "Stop");
		returncode=alert->Go();
		switch(returncode)
		{
			case 0:
				return FS_CLOBBER;
			case 1:
				return FS_SKIP;
			default:
				return B_CANCELED;
		}
	}
	return B_OK;
}

status_t CopyFile(BEntry *src,BEntry *dest, bool clobber)
{
	// Copy a file. Both are expected to be file paths.
	if(!src || !dest || src->IsDirectory() || dest->IsDirectory())
		return B_ERROR;

	status_t status;
	
	// copy file
	BFile *destfile=new BFile;

	if(clobber)
	{
		status=destfile->SetTo(dest, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		if(status!= B_OK)
		{
			delete destfile;
			return status;
		}
	}
	else
	{
		status=destfile->SetTo(dest, B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS);
		if(status!= B_OK)
		{
			delete destfile;
			return status;
		}
	}
	
	BFile *srcfile=new BFile(src, B_READ_ONLY);
	status=srcfile->InitCheck();
	if(status!=B_OK)
	{
		delete srcfile;
		delete destfile;
		return status;
	}
	
	char *buffer=new char[COPY_BUFFER_SIZE];
	ssize_t bytes_read=0;
	
	do
	{
		bytes_read=srcfile->Read((void *) buffer, COPY_BUFFER_SIZE);
		destfile->Write(buffer, bytes_read);
	} while (bytes_read>0);

	delete srcfile;
	delete destfile;

	// copy attributes
	BNode srcnode(src);
	BNode destnode(dest);
	
	srcnode.RewindAttrs();

	int8 *attr_buffer;
	ssize_t attr_size;
	attr_info attr_info;
	char attr_name[B_ATTR_NAME_LENGTH];
	
	while (srcnode.GetNextAttrName(attr_name)==B_OK)
	{
		// first get the size & type of the attribute
		if(srcnode.GetAttrInfo(attr_name, &attr_info) !=B_OK)
			continue;
		
		// allocate memory to hold the attribute
		attr_buffer=new int8[attr_info.size]; 
		if(attr_buffer==NULL)
			continue;
		
		attr_size=srcnode.ReadAttr(attr_name, attr_info.type, 0LL, attr_buffer, attr_info.size);
		
		destnode.WriteAttr(attr_name, attr_info.type, 0LL, attr_buffer, attr_size);
		destnode.Sync();
												
		delete attr_buffer;
	}
	return B_OK;
}

status_t MoveFile(BEntry *src,BEntry *dest, bool clobber)
{
	if(!src || !dest)
		return B_ERROR;
	
	status_t status;
	BDirectory dir;
	BPath path,parent;
	
	// Obtain the destination directory
	dest->GetPath(&path);
	path.GetParent(&parent);
	
	status=dir.SetTo(parent.Path());
	if(status!=B_OK)
		return status;
	
	BString newpath=parent.Path();
	char newname[B_FILE_NAME_LENGTH];
	dest->GetName(newname);
	newpath+="/";
	newpath+=newname;
	status=src->MoveTo(&dir,newpath.String(),clobber);
	
	if(status==B_CROSS_DEVICE_LINK)
	{
		BPath srcpath;
		src->GetPath(&srcpath);
		BString command("mv "),srcstring(srcpath.Path()),deststring(path.Path());
		srcstring.CharacterEscape(" ",'\\');
		deststring.CharacterEscape(" ",'\\');
		command+=srcstring;
		command+=" ";
		command+=deststring;
		system(command.String());
	}
	return status;
}

const char *GetValidName(BEntry *entry)
{
	// given a particular location, this will (1) check to see if said entry
	// exists and if it does, generates a filename which will work complete with
	// the full path. The entry is also set to this new, valid path
	
	BPath path;
	entry->GetPath(&path);

	if(entry->Exists())
	{
		// separate into path and leaf
		char leafbase[B_FILE_NAME_LENGTH];
		char newpath[B_PATH_NAME_LENGTH];
		strcpy(leafbase, path.Leaf());
		path.GetParent(&path);
		
		int32 attempt=1;
		
		do
		{
			if(attempt>1)
				sprintf(newpath, "%s/%s copy %ld", path.Path(),leafbase, attempt);
			else
				sprintf(newpath, "%s/%s copy", path.Path(),leafbase);

			entry->SetTo(newpath);

			attempt++;
		} while (entry->Exists());
		
		return newpath;
	}
	
	return path.Path();
}

bool IsFilenameChar(char c)
{
	const char validstring[]="1234567890-_ ~.,+=!@#$%^&[]{}";
	if( (c>64 && c<91) || (c>96 && c<123))
		return true;
	int validlen=strlen(validstring);
	for(int i=0;i<validlen;i++)
		if(c==validstring[i])
			return true;
	return false;
}

int charcmp(char c1, char c2)
{
	// Case-sensitive character compare
	if(c1<c2)
		return -1;
	else
		if(c2<c1)
			return 1;
	return 0;
}

int charncmp(char c1, char c2)
{
	if(c1>96 && c1<123)
		c1-=32;
	if(c2>96 && c2<123)
		c2-=32;

	if(c1<c2)
		return -1;
	else
		if(c2<c1)
			return 1;
	return 0;
}
