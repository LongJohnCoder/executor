/*
 * Copyright 1997 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

#if !defined (OMIT_RCSID_STRINGS)
char ROMlib_rcsid_ini[] =
		    "$Id: ini.c 88 2005-05-25 03:59:37Z ctm $";
#endif

/*
 * Yahoo -- yet another configuration file thingy.  Who needs YACC when
 * you expect your end user to be familiar with DOS .ini files?
 */

#include "rsys/common.h"
#include <ctype.h>

#include "rsys/ini.h"

typedef struct heading_link_str
{
  struct heading_link_str *next;
  heading_t heading;
  pair_link_t *pairs;
}
heading_link_t;

PRIVATE heading_link_t *headings;

PUBLIC heading_t
new_heading (unsigned char *start, int len)
{
  heading_t retval;
  heading_link_t *linkp;

  linkp = malloc (sizeof *linkp);
  if (!linkp)
    retval = NULL;
  else
    {
      linkp->heading = malloc (len + 1);
      if (!linkp->heading)
	{
	  retval = NULL;
	  free (linkp);
	}
      else
	{
	  strncpy (linkp->heading, (char *) start, len);
	  linkp->heading[len] = 0;
	  linkp->next = headings;
	  linkp->pairs = 0;
	  headings = linkp;
	  retval = linkp->heading;
	}
    }

  return retval;
}

PRIVATE heading_link_t *
find_heading (heading_t heading)
{
  static heading_link_t *cachep;
  heading_link_t *retval;

  if (cachep && strcmp (cachep->heading, heading) == 0)
    retval = cachep;
  else
    {
      for (retval = headings;
	   retval && strcmp (retval->heading, heading) != 0;
	   retval = retval->next)
	;
      if (retval)
	cachep = retval;
    }
  return retval;
}

PUBLIC void
new_key_value_pair (heading_t heading, unsigned char *keystart, int keylen,
		    unsigned char *valuestart, int valuelen)
{
  pair_link_t *pairp;

  pairp = malloc (sizeof *pairp);
  if (pairp)
    {
      pairp->key = malloc (keylen+1);
      if (!pairp->key)
	free (pairp);
      else
	{
	  pairp->value = malloc (valuelen+1);
	  if (!pairp->value)
	    {
	      free (pairp->key);
	      free (pairp);
	    }
	  else
	    {
	      heading_link_t *headingp;

	      strncpy (pairp->key, (char *) keystart, keylen);
	      pairp->key[keylen] = 0;
	      strncpy (pairp->value, (char *) valuestart, valuelen);
	      pairp->value[valuelen] = 0;
	      headingp = find_heading (heading);
	      pairp->next = 0;
	      if (!headingp)
		{
		  warning_unexpected ("couldn't find %s", heading);
		  free (pairp->key);
		  free (pairp->value);
		  free (pairp);
		}
	      else
		{
		  pair_link_t **pairpp;

		  for (pairpp = &headingp->pairs;
		       *pairpp;
		       pairpp = &(*pairpp)->next)
		    ;
		  *pairpp = pairp;
		}
	    }
	}
    }
}

#if 0
/* This routine is currently unsafe to use because it might result in
   dangling pointers */
PUBLIC void
discard_all_inis (void)
{
  heading_link_t *headerp, *nextheaderp;

  for (headerp = headings; headerp; headerp = nextheaderp)
    {
      nextheaderp = headerp->next;
      {
	pair_link_t *pairp, *nextpairp;

	for (pairp = headerp->pairs; pairp; pairp = nextpairp)
	  {
	    nextpairp = pairp->next;
	    free (pairp->key);
	    free (pairp->value);
	    free (pairp);
	  }
      }
      free (headerp);
    }
}
#endif

PUBLIC boolean_t
read_ini_file (const char *filename)
{
  FILE *fp;
  boolean_t retval;

  fp = fopen (filename, "r");
  if (fp)
    {
      heading_t heading;
      int line;

      heading = "";
      line = 0;
      while (!feof (fp))
	{
	  unsigned char buf[1024];
	  unsigned char *p;

	  ++line;
	  if (!fgets ((char *) buf, sizeof buf, fp))
	    buf[0] = 0;

	  for (p = buf; *p && isspace (*p); ++p)
	    ;
	  switch (*p)
	    {
	    case '[':
	      {
		unsigned char *ep;

		++p;
		ep = (unsigned char *) strrchr ((char *) p, ']');
		if (*ep)
		  heading = new_heading (p, ep - p);
		else
		  warning_unexpected ("missing ] on line %d", line);
	      }
	      break;
	    case '#':
	    case 0:
	      break;
	    default:
	      {
		unsigned char *eq;

		eq = (unsigned char *) strchr ((char *) p, '=');
		if (!eq)
		  warning_unexpected ("missing = on line %d", line);
		else
		  {
		    unsigned char *ep;

		    ep = p + strlen ((char *) p);
		    while (isspace (*(ep-1)))
		      --ep;
		    new_key_value_pair (heading, p, eq - p, eq+1, ep-eq-1);
		  }
	      }
	      break;
	    }
	}
    }
  retval = !!fp;
  return retval;
}

PUBLIC pair_link_t *
get_pair_link_n (heading_t heading, int n)
{
  pair_link_t *retval;
  heading_link_t *headingp;

  retval = NULL;
  headingp = find_heading (heading);
  if (headingp)
    {
      retval = headingp->pairs;
      while (retval && n > 0)
	{
	  retval = retval->next;
	  --n;
	}
    }

  return retval;
}

PUBLIC FILE *
open_ini_file_for_writing (const char *filename)
{
  FILE *retval;
  
  retval = fopen (filename, "w");
  return retval;
}

PUBLIC boolean_t
add_heading_to_file (FILE *fp, heading_t heading)
{
  boolean_t retval;

  retval = fprintf (fp, "[%s]\n", heading) > 0;
  return retval;
}

PUBLIC boolean_t
add_key_value_to_file (FILE *fp, ini_key_t key, value_t value)
{
  boolean_t retval;

  retval = fprintf (fp, "%s=%s\n", key, value) > 0;
  return retval;
}

PUBLIC boolean_t
close_ini_file (FILE *fp)
{
  boolean_t retval;

  retval = fclose (fp) == 0;
  return retval;
}

PUBLIC value_t
find_key (heading_t heading, ini_key_t key)
{
  value_t retval;
  heading_link_t *headingp;

  retval = NULL;
  if (heading && key)
    {
      headingp = find_heading (heading);
      if (headingp)
	{
	  pair_link_t *pairp;

	  for (pairp = headingp->pairs;
	       pairp && strcmp (pairp->key, key) != 0;
	       pairp = pairp->next)
	    ;
	  if (pairp)
	    retval = pairp->value;
	}
    }
  return retval;
}
