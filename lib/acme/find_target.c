/*
    NAME
        find_target.c - Find a target given some specification
    DESCRIPTION        
        resolve_filespec() - Finds some objects referred to by a filespec
        find_targets()     - Find object[s] according to some priorities
        find_target()      - Same thing, but only returns one object.
        find_inherit()     - find all the objects that inherit a target file

        The priority string used by find_target() and find_targets()
        is a string of the characters defined in AcmeFindTargetInc.
        You can use literal strings like "ieo", or you can construct
        strings using the #define'd terms like this:
                       FT_InvItem FT_EnvItem FT_FindObject
        Use whichever you find more readable.

    NOTES
        None.
    LAST MODIFIED
        Devo 980109
*/


#include <acme.h>


inherit AcmeError;
inherit AcmeFile;
inherit AcmeForceLoad;
inherit AcmeStrings;

#include AcmeErrorInc
#include AcmeFileInc
#include AcmeForceLoadInc
#include AcmeStringsInc
#include AcmeFindTargetInc

/*----------------------------- resolve_filespec ---------------------------*/
/*
    Description:
      Finds some objects referred to by filename.
    Parameters:
      arg   - A filename or filespec
      quiet - True if error messages should be avoided
    Returns:
      An array of objects referred to by arg.
    Notes:
      May print error messages if quiet == 0 and files can't be found.
*/
object *
resolve_filespec( string arg, status quiet )
{
  mixed mtmp;
  object otmp;
  object *Otmp;
  string *files;

    arg = expand_path(arg,THISP);

    if(arg[<1] == '*')
    {
      if(arg[0] == '/')
        arg = arg[1..<2];
      else
        arg = arg[0..<2];

      mtmp = find_objects(arg);
      return mtmp[1..<1];
    }

    if(otmp = find_object(arg))
      return ({ otmp });

    files = THISP->ExpandPattern(arg, 1);

    if(!sizeof(files))
    {
      if((member(arg, '*') != -1) || (member(arg, '?') != -1))
      {
        if( !quiet ) 
          error(ERR_ERROR, "No files match spec "+to_string(files[0]));
        return 0;
      }

      if(file_exists(arg+".c"))
        return ({ force_load(arg) });

      if( !quiet )
        error(ERR_ERROR, "File "+ arg +" not found.");
      return 0;
    }
    Otmp = force_load_array(files);
    if(pointerp(Otmp) && sizeof(Otmp))
      return Otmp;

    return 0;
}


/*----------------------------- find_targets ---------------------------*/
/*
    Description:
      Finds some objects given a spec and a set of seek priorities
    Parameters:
      spec       - A term in the ospec that wasn't explicit
      priorities - A set of priorities to guide the order and weight of tests
      quiet      - True if error messages should be avoided
      prev       - An array of objects to use as context
    Returns:
      An array of objects referred to by spec in the context of prev,
      according to the specified priorities.
      Returns 0 if given invalid parameters.
    Notes:
      Side effects: May call id() in many objects.
      prev is not used for anything, but is there so that anything
      that overrides this function can choose to use it.
*/
varargs object *
find_targets(string spec, string priorities, status quiet, object *prev )
{
  object ob;
  object *obs;
  int i, len;
  
  if( !stringp( spec ) || !stringp( priorities ) )
    return 0;
  
  for( len = strlen( priorities ); i < len; i++ )
  {
    switch ( priorities[i] )
    {
      case FT_FindPlayerChar:
        ob = FINDP( spec );
        break;
      case FT_FindLivingChar:
        ob = FINDL( spec );
        break;
      case FT_FindObjectChar:
        ob = FINDO( spec );
        break;
      case FT_FileChar:
        obs = resolve_filespec( spec, quiet );
        break;
      case FT_InvItemChar:
        ob = present( spec, THISP );
        break;
      case FT_EnvItemChar:
        if( THISP && ENV(THISP) )
          ob = present(spec, ENV(THISP));
        break;
      default:
        if( !quiet )
          error ( ERR_ERROR, "find_target: invalid priority string");
        return 0;
    }
    if( ob )
      return ({ ob });
    if( obs )
      return obs;
  }
  return ({ });
}


/*----------------------------- find_target ---------------------------*/
/*
    Description:
      Finds an object given a spec and a set of seek priorities      
    Parameters:
      spec       - A term in the ospec that wasn't explicit
      priorities - A set of priorities to guide the order and weight of tests
    Returns:
      The object, or 0 if there was an error or it couldn't find it.
    Notes:
      This function exists for backward compatability.
*/
object 
find_target( string spec, string priorities )
{
  object *obs;
  obs = find_targets( spec, priorities, 0, 0 );
  if ( !obs || !sizeof( obs ) )
    return 0;
  return obs[0];
}

/*------------------------- find_inherit ------------------------------*/
/*
    Description:
      Find all the objects that inherit a target file
    Parameters:
      name  - the filename of the files you want to find (like find_objects())
      iname - the filename of the inheritable to check against
    Returns:
      A list of objects that inherit iname.
    Notes:
      None.
*/
 
#include <driver_config.h>

static private closure find_inherit_cl;
 
object *
find_inherit( string name, string iname )
{
  object *ret, *stuff;
  object last_ob;
  if( name[0]=='/' && strlen(name) > 1)
    name = name[1..];
  iname = munge_filename( iname );
  if( !closurep( find_inherit_cl ) )
    find_inherit_cl = lambda( ({ 'x, 'y }),
      ({ #'!=,
        ({ #'member,
          ({ #'inherit_list, 'x }),
          'y
        }),
        -1
      }) );
 
  last_ob = 0;
  ret = ({ });
  do
  {
    int j, x;
    if( last_ob )
      stuff = find_objects( name, last_ob );
    else
      stuff = find_objects( name );

    j = (int)stuff[0];
    stuff = filter_array( stuff[1..], find_inherit_cl, iname );
    if( (x=sizeof(ret)) + sizeof(stuff) > MAX_ARRAY_SIZE )
    {
      ret += stuff[0..<(MAX_ARRAY_SIZE-x-1)];
      break;
    }
    ret += stuff;
    if( j ) 
      last_ob = stuff[<1];
    else last_ob = 0;
  }
  while(last_ob);
  return ret;
}  
