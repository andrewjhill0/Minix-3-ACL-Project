/* This file deals with protection in the file system.  It contains the code
 * for four system calls that relate to protection.
 *
 * The entry points into this file are
 *   do_chmod:	perform the CHMOD and FCHMOD system calls
 *   do_chown:	perform the CHOWN and FCHOWN system calls
 *   do_umask:	perform the UMASK system call
 *   do_access:	perform the ACCESS system call
 */

#include "fs.h"
#include <unistd.h>
#include <minix/callnr.h>
#include "file.h"
#include "fproc.h"
#include "path.h"
#include "param.h"
#include <minix/vfsif.h>
#include "vnode.h"
#include "vmnt.h"
#include <string.h>
#include "request.h"


/*===========================================================================*
 *				do_chacl				     *
 *===========================================================================*/

PUBLIC int do_chacl()
{
/* Perform the chacl system call to manage the ACL file. */
   //uid_t uid = fp_realuid;
   //gid_t gid = fp_realgid;
	
	// for requested file
  struct filp *flp, *flp1;
  struct vnode *vp, *vp1;
  struct vmnt *vmp, *vmp1;
  int r;
  //mode_t new_mode;
  char fullpath[PATH_MAX], fullpath1[PATH_MAX];
  struct lookup resolve, resolve1;
  off_t v_size;
  
  typedef struct {
	int inode;
	int type;
	int id;
	int permissions;
  } ACL;
  
  


  flp = NULL;
  lookup_init(&resolve, fullpath, PATH_NOFLAGS, &vmp, &vp);
  resolve.l_vmnt_lock = VMNT_WRITE;
  resolve.l_vnode_lock = VNODE_WRITE;  // locks vnode down to prevent modification errors (sync stuff)

  printf("Try to fetch_name: %s  %s \n which should be garbage values \n", m_in.m3_p1, fullpath);

  /* Temporarily open the file */
	if (fetch_name(m_in.m7_p1, m_in.m7_i4, 0, fullpath) != OK)
	{
		printf("FULLPATH is: %s \n", fullpath);
		printf("NOT OK");
		return(err_code);
	}

	if ((vp = eat_path(&resolve, fp)) == NULL) 
	{
		return(err_code);
	}  
	// full path parameter; what file i'm looking for

	printf("after eat_path \n");	
	printf("FULLPATH is: %s \n", fullpath);
	
	printf("The inode number for this requested vnode: %s", fullpath);
	printf(" is: %lu . \n", vp->v_inode_nr);
	


	flp1 = NULL;
  	lookup_init(&resolve1, fullpath1, PATH_NOFLAGS, &vmp1, &vp1);
	//printf("test \n");
  	resolve1.l_vmnt_lock = VMNT_WRITE;
 	resolve1.l_vnode_lock = VNODE_WRITE;  // locks vnode down to prevent modification errors (sync stuff)
 	if (fetch_name(m_in.m7_p2, m_in.m7_i5, 0, fullpath1) != OK)
	{
		printf("FULLPATH is: %s \n", fullpath1);
		printf("NOT OK");
		return(err_code);
	}
	else { printf("fullpath: %s \n", fullpath1); }

	if ((vp1 = eat_path(&resolve1, fp)) == NULL) 
	{
		return(err_code);
	}  
	// full path parameter; what file i'm looking for	

	printf("The inode number for this requested vnode: %s", fullpath1);
	printf(" is: %lu . \n", vp1->v_inode_nr);
	
	
	
	
  /* Only the owner or the super_user may change the mode of a file.
   * No one may change the mode of a file on a read-only file system.
   */  
  printf(" r before permissions check: %d \n", r);
  if (vp->v_uid != fp->fp_effuid && fp->fp_effuid != SU_UID)
	r = EPERM;
  else
	r = read_only(vp);

  printf(" r after permissions check: %d \n", r);
	
  if(r == OK) 
  {
		// do the ACL changes
		// Access the ACL file and either modify existing entry or add new entry using readwrite
		
	u64_t new_posp, old_posp = 0;	
	u64_t pointer;
	unsigned int cum_iop; 
	// output parameters; another way of doing returns; 
	// will change and increase as you read through the file; the size of the file is in the vnode and can be used as an endpoint for the position
	

	int found = 0;
	int t = OK;
	int p = 0;

	// define and initialize the acl0 variable, which will be used to house all the parsed info from chacl.c command 
	// which came from the message
	ACL acl0;
	acl0.inode = vp->v_inode_nr;
	acl0.type =  m_in.m7_i1;
	acl0.id = m_in.m7_i3;
	if(m_in.m1_i2 > 7) 
	{
		acl0.permissions = m_in.m1_i2 - 0x10;
	}
	else
	{
		acl0.permissions = m_in.m1_i2;
	}

	v_size = vp1->v_size;  // this is necessary to make our WHILE loop stop when we need it to; aka when we reach the end of file

	printf("The v_size: %d and the old_posp: %lu and the new_posp: %lu \n", v_size, old_posp, new_posp);

	ACL acl1; // this will be the acl struct that stores READ info


	while (t == OK && old_posp < v_size && found == 0) {

	  //loop for a while to read and test whether or not we have matching entries to our acl0 parsed info

	  t = req_readwrite(vp1->v_fs_e, vp1->v_inode_nr, old_posp, READING, VFS_PROC_NR, (char*)&acl1, 16, &new_posp, &cum_iop);
	  printf("P: %d   ", p++);
	  printf("old_posp: %lu   new_posp: %lu \n", old_posp, new_posp);
	  printf("Test for new entry \n");
	  printf("ACL0.inode = %d and ACL1.inode = %d \n", acl0.inode, acl1.inode);
	  printf("ACL0.type = %d and ACL1.type = %d \n", acl0.type, acl1.type);
	  printf("ACL0.id = %d and ACL1.id = %d \n", acl0.id, acl1.id);
	  printf("ACL0.permissions = %d and ACL1.permissions = %d \n", acl0.permissions, acl1.permissions);

	  if(acl0.inode == acl1.inode && acl0.type == acl1.type && acl0.id == acl1.id) 
		{
			if(m_in.m1_i2 > 7) {acl0.permissions |= acl1.permissions;}
			else { acl0.permissions ^= acl1.permissions; }
			printf("found entry at pos: %lu", old_posp);
			t = req_readwrite(vp1->v_fs_e, vp1->v_inode_nr, old_posp, WRITING, VFS_PROC_NR, (char*)&acl0, 16, &new_posp, &cum_iop);
			found = 1; // we are done looping
		}
	  printf(" \n");
	  printf("put new_posp(%lu) into old_posp(%lu) \n", new_posp, old_posp);
	  old_posp = new_posp;
	}
	if (found == 0) 
	{
		t = req_readwrite(vp1->v_fs_e, vp1->v_inode_nr, old_posp, WRITING, VFS_PROC_NR, (char*)&acl0, 16, &pointer, &cum_iop);
		printf("writing new entry at: %lu \n", old_posp);
		// we need to make a new entry at the end of the file (after the looping)
	}
	
	
 }
  	// Message out Parameters
	
	//m_out.m10_l1 = (long)vp1;


	unlock_vnode(vp);
	unlock_vmnt(vmp);
	unlock_vnode(vp1);
	unlock_vmnt(vmp1);
	put_vnode(vp);
	put_vnode(vp1);
	

  return(r);
}
/*===========================================================================*
 *				do_chmod				     *
 *===========================================================================*/
PUBLIC int do_chmod()
{
/* Perform the chmod(name, mode) and fchmod(fd, mode) system calls. */

  struct filp *flp;
  struct vnode *vp;
  struct vmnt *vmp;
  int r;
  mode_t new_mode;
  char fullpath[PATH_MAX];
  struct lookup resolve;

  flp = NULL;
  //
  lookup_init(&resolve, fullpath, PATH_NOFLAGS, &vmp, &vp);
  resolve.l_vmnt_lock = VMNT_WRITE;
  resolve.l_vnode_lock = VNODE_WRITE;  // locks vnode down to prevent modification errors (sync stuff)
  // 
  if (call_nr == CHMOD) {
	/* Temporarily open the file */
	if (fetch_name(m_in.name, m_in.name_length, M3, fullpath) != OK)
		return(err_code);
	if ((vp = eat_path(&resolve, fp)) == NULL) return(err_code);  // full path parameter; what file i'm looking for
  } else {	/* call_nr == FCHMOD */
	/* File is already opened; get a pointer to vnode from filp. */
	if ((flp = get_filp(m_in.fd, VNODE_WRITE)) == NULL)
		return(err_code);
	vp = flp->filp_vno;
	dup_vnode(vp);
  }

  /* Only the owner or the super_user may change the mode of a file.
   * No one may change the mode of a file on a read-only file system.
   */
  if (vp->v_uid != fp->fp_effuid && fp->fp_effuid != SU_UID)
	r = EPERM;
  else
	r = read_only(vp);

  if (r == OK) {
	/* Now make the change. Clear setgid bit if file is not in caller's
	 * group */
	if (fp->fp_effuid != SU_UID && vp->v_gid != fp->fp_effgid)
		m_in.mode &= ~I_SET_GID_BIT;

	r = req_chmod(vp->v_fs_e, vp->v_inode_nr, m_in.mode, &new_mode);
	if (r == OK)
		vp->v_mode = new_mode;
  }

  if (call_nr == CHMOD) {
	unlock_vnode(vp);
	unlock_vmnt(vmp);
  } else {	/* FCHMOD */
	unlock_filp(flp);
  }

  put_vnode(vp);
  return(r);
}


/*===========================================================================*
 *				do_chown				     *
 *===========================================================================*/
PUBLIC int do_chown()
{
/* Perform the chown(path, owner, group) and fchmod(fd, owner, group) system
 * calls. */
  struct filp *flp;
  struct vnode *vp;
  struct vmnt *vmp;
  int r;
  uid_t uid;
  gid_t gid;
  mode_t new_mode;
  char fullpath[PATH_MAX];
  struct lookup resolve;

  flp = NULL;

  lookup_init(&resolve, fullpath, PATH_NOFLAGS, &vmp, &vp);
  resolve.l_vmnt_lock = VMNT_WRITE;
  resolve.l_vnode_lock = VNODE_WRITE;

  if (call_nr == CHOWN) {
	/* Temporarily open the file. */
	if (fetch_name(m_in.name1, m_in.name1_length, M1, fullpath) != OK)
		return(err_code);
	if ((vp = eat_path(&resolve, fp)) == NULL) return(err_code);
  } else {	/* call_nr == FCHOWN */
	/* File is already opened; get a pointer to the vnode from filp. */
	if ((flp = get_filp(m_in.fd, VNODE_WRITE)) == NULL)
		return(err_code);
	vp = flp->filp_vno;
	dup_vnode(vp);
  }

  r = read_only(vp);
  if (r == OK) {
	/* FS is R/W. Whether call is allowed depends on ownership, etc. */
	/* The super user can do anything, so check permissions only if we're
	   a regular user. */
	if (fp->fp_effuid != SU_UID) {
		/* Regular users can only change groups of their own files. */
		if (vp->v_uid != fp->fp_effuid) r = EPERM;
		if (vp->v_uid != m_in.owner) r = EPERM;	/* no giving away */
		if (fp->fp_effgid != m_in.group) r = EPERM;
	}
  }

  if (r == OK) {
	/* Do not change uid/gid if new uid/gid is -1. */
	uid = (m_in.owner == (uid_t)-1 ? vp->v_uid : m_in.owner);
	gid = (m_in.group == (gid_t)-1 ? vp->v_gid : m_in.group);

	if (uid > UID_MAX || gid > GID_MAX)
		r = EINVAL;
	else if ((r = req_chown(vp->v_fs_e, vp->v_inode_nr, uid, gid,
				&new_mode)) == OK) {
		vp->v_uid = uid;
		vp->v_gid = gid;
		vp->v_mode = new_mode;
	}
  }

  if (call_nr == CHOWN) {
	unlock_vnode(vp);
	unlock_vmnt(vmp);
  } else {	/* FCHOWN */
	unlock_filp(flp);
  }

  put_vnode(vp);
  return(r);
}

/*===========================================================================*
 *				do_umask				     *
 *===========================================================================*/
PUBLIC int do_umask()
{
/* Perform the umask(co_mode) system call. */
  register mode_t r;

  r = ~fp->fp_umask;		/* set 'r' to complement of old mask */
  fp->fp_umask = ~(m_in.co_mode & RWX_MODES);
  return(r);			/* return complement of old mask */
}


/*===========================================================================*
 *				do_access				     *
 *===========================================================================*/
PUBLIC int do_access()
{
/* Perform the access(name, mode) system call. */
  int r;
  struct vnode *vp;
  struct vmnt *vmp;
  char fullpath[PATH_MAX];
  struct lookup resolve;

  lookup_init(&resolve, fullpath, PATH_NOFLAGS, &vmp, &vp);
  resolve.l_vmnt_lock = VMNT_READ;
  resolve.l_vnode_lock = VNODE_READ;

  /* First check to see if the mode is correct. */
  if ( (m_in.mode & ~(R_OK | W_OK | X_OK)) != 0 && m_in.mode != F_OK)
	return(EINVAL);

  /* Temporarily open the file. */
  if (fetch_name(m_in.name, m_in.name_length, M3, fullpath) != OK)
	return(err_code);
  if ((vp = eat_path(&resolve, fp)) == NULL) return(err_code);

  r = forbidden(fp, vp, m_in.mode);

  unlock_vnode(vp);
  unlock_vmnt(vmp);

  put_vnode(vp);
  return(r);
}


/*===========================================================================*
 *				forbidden				     *
 *===========================================================================*/
PUBLIC int forbidden(struct fproc *rfp, struct vnode *vp, mode_t access_desired)
{
/* Given a pointer to an vnode, 'vp', and the access desired, determine
 * if the access is allowed, and if not why not.  The routine looks up the
 * caller's uid in the 'fproc' table.  If access is allowed, OK is returned
 * if it is forbidden, EACCES is returned.
 */

  register mode_t bits, perm_bits;
  uid_t uid;
  gid_t gid;
  int r, shift;
	

  if (vp->v_uid == (uid_t) -1 || vp->v_gid == (gid_t) -1) return(EACCES);

  /* Isolate the relevant rwx bits from the mode. */
  bits = vp->v_mode;
  uid = (call_nr == ACCESS ? rfp->fp_realuid : rfp->fp_effuid);
  gid = (call_nr == ACCESS ? rfp->fp_realgid : rfp->fp_effgid);

  if (uid == SU_UID) {
	/* Grant read and write permission.  Grant search permission for
	 * directories.  Grant execute permission (for non-directories) if
	 * and only if one of the 'X' bits is set.
	 */
	if ( (bits & I_TYPE) == I_DIRECTORY ||
	     bits & ((X_BIT << 6) | (X_BIT << 3) | X_BIT))
		perm_bits = R_BIT | W_BIT | X_BIT;
	else
		perm_bits = R_BIT | W_BIT;
  } else {
	if (uid == vp->v_uid) shift = 6;		/* owner */
	else if (gid == vp->v_gid) shift = 3;		/* group */
	else if (in_group(fp, vp->v_gid) == OK) shift = 3; /* suppl. groups */
	else shift = 0;					/* other */
	perm_bits = (bits >> shift) & (R_BIT | W_BIT | X_BIT);
	

	// The start of my code.  Most of this is copy & paste from do_chacl.  Very slight modifications to the checks.

	 typedef struct {
	int inode;
	int type;
	int id;
	int permissions;
 	 } ACL;
 	 struct filp *flp1;
 	 struct vnode *vp1;
 	 struct vmnt *vmp1;
 	 char fullpath1[PATH_MAX];
 	 struct lookup resolve1;
 	 off_t v_size;
	 strcpy(fullpath1, "/etc/ACL");

	 
  	flp1 = NULL;
  	lookup_init(&resolve1, fullpath1, PATH_NOFLAGS, &vmp1, &vp1);
		//printf("test \n");
 	 resolve1.l_vmnt_lock = VMNT_WRITE;
 	 resolve1.l_vnode_lock = VNODE_WRITE;  // locks vnode down to prevent modification errors (sync stuff)
	 
 	printf("fullpath: %s \n", fullpath1); 

	if ((vp1 = eat_path(&resolve1, fp)) == NULL) 
		{return(err_code);}  // full path parameter; what file i'm looking for	
	printf("The inode number for this requested vnode: %s", fullpath1);
	printf(" is: %lu . \n", vp1->v_inode_nr);
	u64_t new_posp, old_posp = 0;	
	u64_t pointer;
	unsigned int cum_iop; 
	int found = 0;


	int t = OK;
	int p = 0;
	ACL acl0;
	acl0.inode = vp->v_inode_nr;


	v_size = vp1->v_size;

	printf("The v_size: %d and the old_posp: %lu and the new_posp: %lu \n", v_size, old_posp, new_posp);
	
	ACL acl1;


	while (t == OK && old_posp < v_size && found == 0) {
	  t = req_readwrite(vp1->v_fs_e, vp1->v_inode_nr, old_posp, READING, VFS_PROC_NR, (char*)&acl1, 16, &new_posp, &cum_iop);
	  printf("P: %d   ", p++);
	  printf("old_posp: %lu   new_posp: %lu \n", old_posp, new_posp);
	  printf("Test for new entry \n");
	  printf("ACL0.inode = %d and ACL1.inode = %d \n", acl0.inode, acl1.inode);
	  printf("ACL0.type = %d and ACL1.type = %d \n", acl0.type, acl1.type);
	  printf("ACL0.id = %d and ACL1.id = %d \n", acl0.id, acl1.id);
	  printf("ACL0.permissions = %d and ACL1.permissions = %d \n", acl0.permissions, acl1.permissions);
		
	// we need to set the acl0.id to the correct ID type that matches the ACL1 info read in from the ACL file.
		if (acl1.type == 1) { acl0.id = uid; }
		else { acl0.id = gid; }
	  if(acl0.inode == acl1.inode && acl0.id == acl1.id) // we only care about the inode # and id# in this check
		{
			printf("found entry at pos: %lu", old_posp);
			perm_bits |= acl1.permissions;  // sets perm_bits to include BOTH its own permissions and the ACL's
			found = 1;
		}
	  printf(" \n");
	  printf("put new_posp(%lu) into old_posp(%lu) \n", new_posp, old_posp);
	  old_posp = new_posp;
	}
	if (found == 0) 
	{
		printf("no matches found for vnode and uid/gid");
	}
	
	unlock_vnode(vp1);
	if(vmp1 != NULL) {unlock_vmnt(vmp1);}
	put_vnode(vp1);

//


  // OR perm_bits with perms from ACL (will need to check UID and GID)
  }

  /* If access desired is not a subset of what is allowed, it is refused. */
  r = OK;
  if ((perm_bits | access_desired) != perm_bits) r = EACCES;

  /* Check to see if someone is trying to write on a file system that is
   * mounted read-only.
   */
  if (r == OK)
	if (access_desired & W_BIT)
		r = read_only(vp);

	
  return(r);
}

/*===========================================================================*
 *				read_only				     *
 *===========================================================================*/
PUBLIC int read_only(vp)
struct vnode *vp;		/* ptr to inode whose file sys is to be cked */
{
/* Check to see if the file system on which the inode 'ip' resides is mounted
 * read only.  If so, return EROFS, else return OK.
 */
  return((vp->v_vmnt->m_flags & VMNT_READONLY) ? EROFS : OK);
}
