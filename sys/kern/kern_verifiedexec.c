/*	$NetBSD: kern_verifiedexec.c,v 1.78.2.9 2007/03/10 12:18:34 bouyer Exp $	*/

/*-
 * Copyright 2005 Elad Efrat <elad@NetBSD.org>
 * Copyright 2005 Brett Lymn <blymn@netbsd.org>
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Brett Lymn and Elad Efrat
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_verifiedexec.c,v 1.78.2.9 2007/03/10 12:18:34 bouyer Exp $");

#include "opt_veriexec.h"

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/syslog.h>
#include <sys/sysctl.h>
#include <sys/inttypes.h>
#include <sys/verified_exec.h>
#if defined(__FreeBSD__)
# include <sys/systm.h>
# include <sys/imgact.h>
# include <crypto/sha1.h>
# include <crypto/sha2/sha2.h>
# include <crypto/ripemd160/rmd160.h>
#else
# include <sys/sha1.h>
# include <sys/sha2.h>
# include <sys/rmd160.h>
#endif
#include <sys/md5.h>
#include <uvm/uvm_extern.h>
#include <sys/fileassoc.h>
#include <sys/kauth.h>
#include <sys/conf.h>
#include <miscfs/specfs/specdev.h>
#include <prop/proplib.h>
#include <sys/fcntl.h>

MALLOC_DEFINE(M_VERIEXEC, "Veriexec", "Veriexec data-structures");

struct veriexec_fpops {
	const char *type;
	size_t hash_len;
	size_t context_size;
	veriexec_fpop_init_t init;
	veriexec_fpop_update_t update;
	veriexec_fpop_final_t final;
	LIST_ENTRY(veriexec_fpops) entries;
};

/* Veriexec per-file entry data. */
struct veriexec_file_entry {
	u_char type;				/* Entry type. */
	u_char status;				/* Evaluation status. */
	u_char page_fp_status;			/* Per-page FP status. */
	u_char *fp;				/* Fingerprint. */
	void *page_fp;				/* Per-page fingerprints */
	size_t npages;			    	/* Number of pages. */
	size_t last_page_size;			/* To support < PAGE_SIZE */
	struct veriexec_fpops *ops;		/* Fingerprint ops vector*/
};

/* Veriexec per-table data. */
struct veriexec_table_entry {
	uint64_t vte_count;			/* Number of Veriexec entries. */
	const struct sysctlnode *vte_node;
};

int veriexec_verbose;
int veriexec_strict;

char *veriexec_fp_names;
size_t veriexec_name_max;

const struct sysctlnode *veriexec_count_node;

int veriexec_hook;

LIST_HEAD(, veriexec_fpops) veriexec_fpops_list;

static int veriexec_raw_cb(kauth_cred_t, kauth_action_t, void *,
    void *, void *, void *, void *);
static int sysctl_kern_veriexec(SYSCTLFN_PROTO);
static struct veriexec_fpops *veriexec_fpops_lookup(const char *);
static void veriexec_clear(void *, int);

static unsigned int veriexec_tablecount = 0;

/*
 * Sysctl helper routine for Veriexec.
 */
static int
sysctl_kern_veriexec(SYSCTLFN_ARGS)
{
	int newval, error;
	int *var = NULL, raise_only = 0;
	struct sysctlnode node;

	node = *rnode;

	if (strcmp(rnode->sysctl_name, "strict") == 0) {
		raise_only = 1;
		var = &veriexec_strict;
	} else if (strcmp(rnode->sysctl_name, "algorithms") == 0) {
		node.sysctl_data = veriexec_fp_names;
		node.sysctl_size = strlen(veriexec_fp_names) + 1;
		return (sysctl_lookup(SYSCTLFN_CALL(&node)));
	} else {
		return (EINVAL);
	}

	newval = *var;

	node.sysctl_data = &newval;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL) {
		return (error);
	}

	if (raise_only && (newval < *var))
		return (EPERM);

	*var = newval;

	return (error);
}

SYSCTL_SETUP(sysctl_kern_veriexec_setup, "sysctl kern.veriexec setup")
{
	const struct sysctlnode *rnode = NULL;

	sysctl_createv(clog, 0, NULL, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "kern", NULL,
		       NULL, 0, NULL, 0,
		       CTL_KERN, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "veriexec",
		       SYSCTL_DESCR("Veriexec"),
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "verbose",
		       SYSCTL_DESCR("Veriexec verbose level"),
		       NULL, 0, &veriexec_verbose, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "strict",
		       SYSCTL_DESCR("Veriexec strict level"),
		       sysctl_kern_veriexec, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "algorithms",
		       SYSCTL_DESCR("Veriexec supported hashing "
				    "algorithms"),
		       sysctl_kern_veriexec, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &rnode, &veriexec_count_node,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "count",
		       SYSCTL_DESCR("Number of fingerprints on mount(s)"),
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);
}

/*
 * Add ops to the fignerprint ops vector list.
 */
int
veriexec_fpops_add(const char *fp_type, size_t hash_len, size_t ctx_size,
    veriexec_fpop_init_t init, veriexec_fpop_update_t update,
    veriexec_fpop_final_t final)
{
	struct veriexec_fpops *ops;
	char *newp;
	unsigned int new_max;

	/* Sanity check all parameters. */
	if ((fp_type == NULL) || (hash_len == 0) || (ctx_size == 0) ||
	    (init == NULL) || (update == NULL) || (final == NULL))
		return (EFAULT);

	if (veriexec_fpops_lookup(fp_type) != NULL)
		return (EEXIST);

	ops = malloc(sizeof(*ops), M_VERIEXEC, M_WAITOK);

	ops->type = fp_type;
	ops->hash_len = hash_len;
	ops->context_size = ctx_size;
	ops->init = init;
	ops->update = update;
	ops->final = final;

	LIST_INSERT_HEAD(&veriexec_fpops_list, ops, entries);

	/*
	 * If we don't have space for any names, allocate enough for six
	 * which should be sufficient. (it's also enough for all algorithms
	 * we can support at the moment)
	 */
	if (veriexec_fp_names == NULL) {
		veriexec_name_max = 64;
		veriexec_fp_names = malloc(veriexec_name_max, M_VERIEXEC,
		    M_WAITOK|M_ZERO);
	}

	/*
	 * If we're running out of space for storing supported algorithms,
	 * extend the buffer with space for four names.
	 */
	while (veriexec_name_max - (strlen(veriexec_fp_names) + 1) <
	    strlen(fp_type)) {
		/* Add space for four algorithm names. */
		new_max = veriexec_name_max + 64;
		newp = realloc(veriexec_fp_names, new_max, M_VERIEXEC,
		    M_WAITOK|M_ZERO);
		veriexec_fp_names = newp;
		veriexec_name_max = new_max;
	}

	if (*veriexec_fp_names != '\0')
		strlcat(veriexec_fp_names, " ", veriexec_name_max);

	strlcat(veriexec_fp_names, fp_type, veriexec_name_max);

	return (0);
}

/*
 * Initialise Veriexec.
 */
void
veriexec_init(void)
{
	/* Register a fileassoc for Veriexec. */
	veriexec_hook = fileassoc_register("veriexec", veriexec_clear);
	if (veriexec_hook == FILEASSOC_INVAL)
		panic("Veriexec: Can't register fileassoc");

	/* Register listener to handle raw disk access. */
	if (kauth_listen_scope(KAUTH_SCOPE_DEVICE, veriexec_raw_cb, NULL) ==
	    NULL)
		panic("Veriexec: Can't listen on device scope");

	LIST_INIT(&veriexec_fpops_list);
	veriexec_fp_names = NULL;
	veriexec_name_max = 0;

#define	FPOPS_ADD(a, b, c, d, e, f)	\
	veriexec_fpops_add(a, b, c, (veriexec_fpop_init_t)d, \
	 (veriexec_fpop_update_t)e, (veriexec_fpop_final_t)f)

#ifdef VERIFIED_EXEC_FP_RMD160
	FPOPS_ADD("RMD160", RMD160_DIGEST_LENGTH, sizeof(RMD160_CTX),
	    RMD160Init, RMD160Update, RMD160Final);
#endif /* VERIFIED_EXEC_FP_RMD160 */

#ifdef VERIFIED_EXEC_FP_SHA256
	FPOPS_ADD("SHA256", SHA256_DIGEST_LENGTH, sizeof(SHA256_CTX),
	    SHA256_Init, SHA256_Update, SHA256_Final);
#endif /* VERIFIED_EXEC_FP_SHA256 */

#ifdef VERIFIED_EXEC_FP_SHA384
	FPOPS_ADD("SHA384", SHA384_DIGEST_LENGTH, sizeof(SHA384_CTX),
	    SHA384_Init, SHA384_Update, SHA384_Final);
#endif /* VERIFIED_EXEC_FP_SHA384 */

#ifdef VERIFIED_EXEC_FP_SHA512
	FPOPS_ADD("SHA512", SHA512_DIGEST_LENGTH, sizeof(SHA512_CTX),
	    SHA512_Init, SHA512_Update, SHA512_Final);
#endif /* VERIFIED_EXEC_FP_SHA512 */

#ifdef VERIFIED_EXEC_FP_SHA1
	FPOPS_ADD("SHA1", SHA1_DIGEST_LENGTH, sizeof(SHA1_CTX),
	    SHA1Init, SHA1Update, SHA1Final);
#endif /* VERIFIED_EXEC_FP_SHA1 */

#ifdef VERIFIED_EXEC_FP_MD5
	FPOPS_ADD("MD5", MD5_DIGEST_LENGTH, sizeof(MD5_CTX),
	    MD5Init, MD5Update, MD5Final);
#endif /* VERIFIED_EXEC_FP_MD5 */

#undef FPOPS_ADD
}

static struct veriexec_fpops *
veriexec_fpops_lookup(const char *name)
{
	struct veriexec_fpops *ops;

	if (name == NULL)
		return (NULL);

	LIST_FOREACH(ops, &veriexec_fpops_list, entries) {
		if (strcasecmp(name, ops->type) == 0)
			return (ops);
	}

	return (NULL);
}

/*
 * Calculate fingerprint. Information on hash length and routines used is
 * extracted from veriexec_hash_list according to the hash type.
 */
static int
veriexec_fp_calc(struct lwp *l, struct vnode *vp,
    struct veriexec_file_entry *vfe, u_char *fp)
{
	struct vattr va;
	void *ctx, *page_ctx;
	u_char *buf, *page_fp;
	off_t offset, len;
	size_t resid, npages;
	int error, do_perpage, pagen;

	error = VOP_GETATTR(vp, &va, l->l_cred, l);
	if (error)
		return (error);

#if 0 /* XXX - for now */
	if ((vfe->type & VERIEXEC_UNTRUSTED) &&
	    (vfe->page_fp_status == PAGE_FP_NONE))
		do_perpage = 1;
	else
#endif
		do_perpage = 0;

	ctx = (void *) malloc(vfe->ops->context_size, M_VERIEXEC, M_WAITOK);
	buf = (u_char *) malloc(PAGE_SIZE, M_VERIEXEC, M_WAITOK);

	page_ctx = NULL;
	page_fp = NULL;
	npages = 0;
	if (do_perpage) {
		npages = (va.va_size >> PAGE_SHIFT) + 1;
		page_fp = (u_char *) malloc(vfe->ops->hash_len * npages,
		    M_VERIEXEC, M_WAITOK|M_ZERO);
		vfe->page_fp = page_fp;
		page_ctx = (void *) malloc(vfe->ops->context_size, M_VERIEXEC,
		    M_WAITOK);
	}

	(vfe->ops->init)(ctx);

	len = 0;
	error = 0;
	pagen = 0;
	for (offset = 0; offset < va.va_size; offset += PAGE_SIZE) {
		len = ((va.va_size - offset) < PAGE_SIZE) ?
		    (va.va_size - offset) : PAGE_SIZE;

		error = vn_rdwr(UIO_READ, vp, buf, len, offset,
				UIO_SYSSPACE,
#ifdef __FreeBSD__
				IO_NODELOCKED,
#else
				0,
#endif
				l->l_cred, &resid, NULL);

		if (error) {
			if (do_perpage) {
				free(vfe->page_fp, M_VERIEXEC);
				vfe->page_fp = NULL;
			}

			goto bad;
		}

		(vfe->ops->update)(ctx, buf, (unsigned int) len);

		if (do_perpage) {
			(vfe->ops->init)(page_ctx);
			(vfe->ops->update)(page_ctx, buf, (unsigned int)len);
			(vfe->ops->final)(page_fp, page_ctx);

			if (veriexec_verbose >= 2) {
				int i;

				printf("hash for page %d: ", pagen);
				for (i = 0; i < vfe->ops->hash_len; i++)
					printf("%02x", page_fp[i]);
				printf("\n");
			}

			page_fp += vfe->ops->hash_len;
			pagen++;
		}

		if (len != PAGE_SIZE)
			break;
	}

	(vfe->ops->final)(fp, ctx);

	if (do_perpage) {
		vfe->last_page_size = len;
		vfe->page_fp_status = PAGE_FP_READY;
		vfe->npages = npages;
	}

bad:
	if (do_perpage)
		free(page_ctx, M_VERIEXEC);
	free(ctx, M_VERIEXEC);
	free(buf, M_VERIEXEC);

	return (error);
}

/* Compare two fingerprints of the same type. */
static int
veriexec_fp_cmp(struct veriexec_fpops *ops, u_char *fp1, u_char *fp2)
{
	if (veriexec_verbose >= 2) {
		int i;

		printf("comparing hashes...\n");
		printf("fp1: ");
		for (i = 0; i < ops->hash_len; i++) {
			printf("%02x", fp1[i]);
		}
		printf("\nfp2: ");
		for (i = 0; i < ops->hash_len; i++) {
			printf("%02x", fp2[i]);
		}
		printf("\n");
	}

	return (memcmp(fp1, fp2, ops->hash_len));
}

static struct veriexec_table_entry *
veriexec_table_lookup(struct mount *mp)
{
	return (fileassoc_tabledata_lookup(mp, veriexec_hook));
}

static struct veriexec_file_entry *
veriexec_get(struct vnode *vp)
{
	return (fileassoc_lookup(vp, veriexec_hook));
}

boolean_t
veriexec_lookup(struct vnode *vp)
{
	return (veriexec_get(vp) == NULL ? FALSE : TRUE);
}

/*
 * Verify the fingerprint of the given file. If we're called directly from
 * sys_execve(), 'flag' will be VERIEXEC_DIRECT. If we're called from
 * exec_script(), 'flag' will be VERIEXEC_INDIRECT.  If we are called from
 * vn_open(), 'flag' will be VERIEXEC_FILE.
 */
int
veriexec_verify(struct lwp *l, struct vnode *vp, const u_char *name, int flag,
    boolean_t *found)
{
	struct veriexec_file_entry *vfe;
	u_char *digest;
	int error;

	if (vp->v_type != VREG)
		return (0);

	/* Lookup veriexec table entry, save pointer if requested. */
	vfe = veriexec_get(vp);
	if (found != NULL) {
		if (vfe != NULL)
			*found = TRUE;
		else
			*found = FALSE;
	}
	if (vfe == NULL)
		goto out;

	/* Evaluate fingerprint if needed. */
	error = 0;
	digest = NULL;
	if ((vfe->status == FINGERPRINT_NOTEVAL) ||
	    (vfe->type & VERIEXEC_UNTRUSTED)) {
		/* Calculate fingerprint for on-disk file. */
		digest = (u_char *) malloc(vfe->ops->hash_len, M_VERIEXEC,
		    M_WAITOK);
		error = veriexec_fp_calc(l, vp, vfe, digest);
		if (error) {
			veriexec_report("Fingerprint calculation error.",
			    name, NULL, REPORT_ALWAYS);
			free(digest, M_VERIEXEC);
			return (error);
		}

		/* Compare fingerprint with loaded data. */
		if (veriexec_fp_cmp(vfe->ops, vfe->fp, digest) == 0) {
			vfe->status = FINGERPRINT_VALID;
		} else {
			vfe->status = FINGERPRINT_NOMATCH;
		}

		free(digest, M_VERIEXEC);
	}

	if (!(vfe->type & flag)) {
		veriexec_report("Incorrect access type.", name, l,
		    REPORT_ALWAYS|REPORT_ALARM);

		/* IPS mode: Enforce access type. */
		if (veriexec_strict >= VERIEXEC_IPS)
			return (EPERM);
	}

 out:
	/* No entry in the veriexec tables. */
	if (vfe == NULL) {
		veriexec_report("No entry.", name,
		    l, REPORT_VERBOSE);

		/*
		 * Lockdown mode: Deny access to non-monitored files.
		 * IPS mode: Deny execution of non-monitored files.
		 */
		if ((veriexec_strict >= VERIEXEC_LOCKDOWN) ||
		    ((veriexec_strict >= VERIEXEC_IPS) &&
		     (flag != VERIEXEC_FILE)))
			return (EPERM);

		return (0);
	}

        switch (vfe->status) {
	case FINGERPRINT_NOTEVAL:
		/* Should not happen. */
		veriexec_report("Not-evaluated status "
		    "post evaluation; inconsistency detected.", name,
		    NULL, REPORT_ALWAYS|REPORT_PANIC);

	case FINGERPRINT_VALID:
		/* Valid fingerprint. */
		veriexec_report("Match.", name, NULL,
		    REPORT_VERBOSE);

		break;

	case FINGERPRINT_NOMATCH:
		/* Fingerprint mismatch. */
		veriexec_report("Mismatch.", name,
		    NULL, REPORT_ALWAYS|REPORT_ALARM);

		/* IDS mode: Deny access on fingerprint mismatch. */
		if (veriexec_strict >= VERIEXEC_IDS)
			error = EPERM;

		break;

	default:
		/* Should never happen. */
		veriexec_report("Invalid status "
		    "post evaluation.", name, NULL, REPORT_ALWAYS|REPORT_PANIC);
        }

	return (error);
}

/*
 * Evaluate per-page fingerprints.
 */
int
veriexec_page_verify(struct veriexec_file_entry *vfe, struct vm_page *pg,
    size_t idx, struct lwp *l)
{
	void *ctx;
	u_char *fp;
	u_char *page_fp;
	int error;
	vaddr_t kva;

	if (vfe->page_fp_status == PAGE_FP_NONE)
		return (0);

	if (vfe->page_fp_status == PAGE_FP_FAIL)
		return (EPERM);

	if (idx >= vfe->npages)
		return (0);

	ctx = malloc(vfe->ops->context_size, M_VERIEXEC, M_WAITOK);
	fp = malloc(vfe->ops->hash_len, M_VERIEXEC, M_WAITOK);
	kva = uvm_km_alloc(kernel_map, PAGE_SIZE, 0, UVM_KMF_VAONLY | UVM_KMF_WAITVA);
	pmap_kenter_pa(kva, VM_PAGE_TO_PHYS(pg), VM_PROT_READ);

	page_fp = (u_char *) vfe->page_fp + (vfe->ops->hash_len * idx);
	(vfe->ops->init)(ctx);
	(vfe->ops->update)(ctx, (void *) kva,
			   ((vfe->npages - 1) == idx) ? vfe->last_page_size
						      : PAGE_SIZE);
	(vfe->ops->final)(fp, ctx);

	pmap_kremove(kva, PAGE_SIZE);
	uvm_km_free(kernel_map, kva, PAGE_SIZE, UVM_KMF_VAONLY);

	error = veriexec_fp_cmp(vfe->ops, page_fp, fp);
	if (error) {
		const char *msg;

		if (veriexec_strict > VERIEXEC_LEARNING) {
			msg = "Pages modified: Killing process.";
		} else {
			msg = "Pages modified.";
			error = 0;
		}

		veriexec_report(msg, "[page_in]", l, REPORT_ALWAYS|REPORT_ALARM);

		if (error) {
			ksiginfo_t ksi;

			KSI_INIT(&ksi);
			ksi.ksi_signo = SIGKILL;
			ksi.ksi_code = SI_NOINFO;
			ksi.ksi_pid = l->l_proc->p_pid;
			ksi.ksi_uid = 0;

			kpsignal(l->l_proc, &ksi, NULL);
		}
	}

	free(ctx, M_VERIEXEC);
	free(fp, M_VERIEXEC);

	return (error);
}

/*
 * Veriexec remove policy code.
 */
int
veriexec_removechk(struct vnode *vp, const char *pathbuf, struct lwp *l)
{
	struct veriexec_file_entry *vfe;
	struct veriexec_table_entry *vte;

	vfe = veriexec_get(vp);
	if (vfe == NULL) {
		/* Lockdown mode: Deny access to non-monitored files. */
		if (veriexec_strict >= VERIEXEC_LOCKDOWN)
			return (EPERM);

		return (0);
	}

	veriexec_report("Remove request.", pathbuf, l, REPORT_ALWAYS|REPORT_ALARM);

	/* IDS mode: Deny removal of monitored files. */
	if (veriexec_strict >= VERIEXEC_IDS)
		return (EPERM);

	fileassoc_clear(vp, veriexec_hook);

	vte = veriexec_table_lookup(vp->v_mount);
	KASSERT(vte != NULL);

	vte->vte_count--;

	return (0);
}

/*
 * Veriexe rename policy.
 */
int
veriexec_renamechk(struct vnode *fromvp, const char *fromname,
    struct vnode *tovp, const char *toname, struct lwp *l)
{
	struct veriexec_file_entry *vfe, *tvfe;

	if (veriexec_strict >= VERIEXEC_LOCKDOWN) {
		log(LOG_ALERT, "Veriexec: Preventing rename of `%s' to "
		    "`%s', uid=%u, pid=%u: Lockdown mode.\n", fromname, toname,
		    kauth_cred_geteuid(l->l_cred), l->l_proc->p_pid);
		return (EPERM);
	}

	vfe = veriexec_get(fromvp);
	tvfe = NULL;
	if (tovp != NULL)
		tvfe = veriexec_get(tovp);

	if ((vfe != NULL) || (tvfe != NULL)) {
		if (veriexec_strict >= VERIEXEC_IPS) {
			log(LOG_ALERT, "Veriexec: Preventing rename of `%s' "
			    "to `%s', uid=%u, pid=%u: IPS mode, file "
			    "monitored.\n", fromname, toname,
			    kauth_cred_geteuid(l->l_cred),
			    l->l_proc->p_pid);
			return (EPERM);
		}

		log(LOG_NOTICE, "Veriexec: Monitored file `%s' renamed to "
		    "`%s', uid=%u, pid=%u.\n", fromname, toname,
		    kauth_cred_geteuid(l->l_cred), l->l_proc->p_pid);
	}

	return (0);
}

/*
 * Routine for maintaining mostly consistent message formats in Verified
 * Exec.
 */
void
veriexec_report(const u_char *msg, const u_char *filename, struct lwp *l, int f)
{
	if (msg == NULL || filename == NULL)
		return;

	if (((f & REPORT_LOGMASK) >> 1) <= veriexec_verbose) {
		if (!(f & REPORT_ALARM) || (l == NULL))
			log(LOG_NOTICE, "Veriexec: %s [%s]\n", msg,
			    filename);
		else
			log(LOG_ALERT, "Veriexec: %s [%s, pid=%u, uid=%u, "
			    "gid=%u]\n", msg, filename, l->l_proc->p_pid,
			    kauth_cred_getuid(l->l_cred),
			    kauth_cred_getgid(l->l_cred));
	}

	if (f & REPORT_PANIC)
		panic("Veriexec: Unrecoverable error.");
}

static void
veriexec_clear(void *data, int file_specific)
{
	if (file_specific) {
		struct veriexec_file_entry *vfe = data;

		if (vfe != NULL) {
			if (vfe->fp != NULL)
				free(vfe->fp, M_VERIEXEC);
			if (vfe->page_fp != NULL)
				free(vfe->page_fp, M_VERIEXEC);
			free(vfe, M_VERIEXEC);
		}
	} else {
		struct veriexec_table_entry *vte = data;

		if (vte != NULL)
			free(vte, M_VERIEXEC);
	}
}

/*
 * Invalidate a Veriexec file entry.
 * XXX: This should be updated when per-page fingerprints are added.
 */
static void
veriexec_file_purge(struct veriexec_file_entry *vfe)
{
	if (vfe == NULL)
		return;

	vfe->status = FINGERPRINT_NOTEVAL;
}

void
veriexec_purge(struct vnode *vp)
{
	veriexec_file_purge(veriexec_get(vp));
}

/*
 * Enforce raw disk access policy.
 *
 * IDS mode: Invalidate fingerprints on a mount if it's opened for writing.
 * IPS mode: Don't allow raw writing to disks we monitor.
 * Lockdown mode: Don't allow raw writing to all disks.
 *
 * XXX: This is bogus. There's an obvious race condition between the time
 * XXX: the disk is open for writing, in which an attacker can access a
 * XXX: monitored file to get its signature cached again, and when the raw
 * XXX: file is overwritten on disk.
 * XXX:
 * XXX: To solve this, we need something like the following:
 * XXX:		open raw disk:
 * XXX:		  - raise refcount,
 * XXX:		  - invalidate fingerprints,
 * XXX:		  - mark all entries for that disk with "no cache" flag
 * XXX:
 * XXX:		veriexec_verify:
 * XXX:		  - if "no cache", don't cache evaluation result
 * XXX:
 * XXX:		close raw disk:
 * XXX:		  - lower refcount,
 * XXX:		  - if refcount == 0, remove "no cache" flag from all entries
 */
static int
veriexec_raw_cb(kauth_cred_t cred, kauth_action_t action, void *cookie,
    void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;
	enum kauth_device_req req;
	struct veriexec_table_entry *vte;

	result = KAUTH_RESULT_DENY;
	req = (enum kauth_device_req)arg0;

	switch (action) {
	case KAUTH_DEVICE_RAWIO_SPEC: {
		struct vnode *vp, *bvp;
		dev_t dev;
		int d_type;

		if (req == KAUTH_REQ_DEVICE_RAWIO_SPEC_READ) {
			result = KAUTH_RESULT_DEFER;
			break;
		}

		vp = arg1;
		KASSERT(vp != NULL);

		dev = vp->v_un.vu_specinfo->si_rdev;
		d_type = D_OTHER;
		bvp = NULL;

		/* Handle /dev/mem and /dev/kmem. */
		if ((vp->v_type == VCHR) && iskmemdev(dev)) {
			if (veriexec_strict < VERIEXEC_IPS)
				result = KAUTH_RESULT_DEFER;

			break;
		}

		switch (vp->v_type) {
		case VCHR: {
			const struct cdevsw *cdev;

			cdev = cdevsw_lookup(dev);
			if (cdev != NULL) {
				dev_t blkdev;

				blkdev = devsw_chr2blk(dev);
				if (blkdev != NODEV) {
					vfinddev(blkdev, VBLK, &bvp);
					if (bvp != NULL)
						d_type = cdev->d_type;
				}
			}

			break;
			}
		case VBLK: {
			const struct bdevsw *bdev;

			bdev = bdevsw_lookup(dev);
			if (bdev != NULL)
				d_type = bdev->d_type;

			bvp = vp;

			break;
			}
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}

		if (d_type != D_DISK) {
			result = KAUTH_RESULT_DEFER;
			break;
		}

		/*
		 * XXX: See vfs_mountedon() comment in secmodel/bsd44.
		 */
		vte = veriexec_table_lookup(bvp->v_mount);
		if (vte == NULL) {
			result = KAUTH_RESULT_DEFER;
			break;
		}

		switch (veriexec_strict) {
		case VERIEXEC_LEARNING:
		case VERIEXEC_IDS:
			result = KAUTH_RESULT_DEFER;

			fileassoc_table_run(bvp->v_mount, veriexec_hook,
			    (fileassoc_cb_t)veriexec_file_purge);

			break;
		case VERIEXEC_IPS:
			result = KAUTH_RESULT_DENY;
			break;
		case VERIEXEC_LOCKDOWN:
			result = KAUTH_RESULT_DENY;
			break;
		}

		break;
		}

	case KAUTH_DEVICE_RAWIO_PASSTHRU:
		/* XXX What can we do here? */
		if (veriexec_strict < VERIEXEC_IPS)
			result = KAUTH_RESULT_DEFER;

		break;

	default:
		result = KAUTH_RESULT_DEFER;
		break;
	}

	return (result);
}

/*
 * Add a file to be monitored by Veriexec.
 *
 * Expected elements in dict: file, fp, fp-type, entry-type.
 */
int
veriexec_file_add(struct lwp *l, prop_dictionary_t dict)
{
	struct veriexec_table_entry *vte;
	struct veriexec_file_entry *vfe, *hh;
	struct nameidata nid;
	const char *file, *fp_type;
	int error;

	file = prop_string_cstring_nocopy(prop_dictionary_get(dict, "file"));
	NDINIT(&nid, LOOKUP, FOLLOW, UIO_SYSSPACE, file, l);
	error = namei(&nid);
	if (error)
		return (error);

	/* Add only regular files. */
	if (nid.ni_vp->v_type != VREG) {
		log(LOG_ERR, "Veriexec: Not adding `%s': Not a regular file.\n",
		    file);
		error = EINVAL;
		goto out;
	}

	vfe = malloc(sizeof(*vfe), M_VERIEXEC, M_WAITOK);

	/* Lookup fingerprint hashing algorithm. */
	fp_type = prop_string_cstring_nocopy(prop_dictionary_get(dict,
	    "fp-type"));
	if ((vfe->ops = veriexec_fpops_lookup(fp_type)) == NULL) {
		free(vfe, M_VERIEXEC);
		log(LOG_ERR, "Veriexec: Invalid or unknown fingerprint type "
		    "`%s' for file `%s'.\n", fp_type, file);
		error = EINVAL;
		goto out;
	}

	if (prop_data_size(prop_dictionary_get(dict, "fp")) !=
	    vfe->ops->hash_len) {
		free(vfe, M_VERIEXEC);
		log(LOG_ERR, "Veriexec: Bad fingerprint length for `%s'.\n",
		    file);
		error = EINVAL;
		goto out;
	}

	vfe->fp = malloc(vfe->ops->hash_len, M_VERIEXEC, M_WAITOK);
	memcpy(vfe->fp, prop_data_data_nocopy(prop_dictionary_get(dict, "fp")),
	    vfe->ops->hash_len);

	/*
	 * See if we already have an entry for this file. If we do, then
	 * let the user know and silently pretend to succeed.
	 */
	hh = veriexec_get(nid.ni_vp);
	if (hh != NULL) {
		boolean_t fp_mismatch;

		if (strcmp(vfe->ops->type, fp_type) ||
		    memcmp(hh->fp, vfe->fp, hh->ops->hash_len))
			fp_mismatch = TRUE;
		else
			fp_mismatch = FALSE;

		if ((veriexec_verbose >= 1) || fp_mismatch)
			log(LOG_NOTICE, "Veriexec: Duplicate entry for `%s' "
			    "ignored. (%s fingerprint)\n", file, 
			    fp_mismatch ? "different" : "same");

		free(vfe->fp, M_VERIEXEC);
		free(vfe, M_VERIEXEC);

		error = 0;
		goto out;
	}

	/* Continue entry initialization. */
	prop_dictionary_get_uint8(dict, "entry-type", &vfe->type);
	vfe->status = FINGERPRINT_NOTEVAL;

	vfe->page_fp = NULL;
	vfe->page_fp_status = PAGE_FP_NONE;
	vfe->npages = 0;
	vfe->last_page_size = 0;

	error = fileassoc_add(nid.ni_vp, veriexec_hook, vfe);
	if (error) {
		free(vfe->fp, M_VERIEXEC);
		free(vfe, M_VERIEXEC);
		goto out;
	}

	vte = veriexec_table_lookup(nid.ni_vp->v_mount);
	vte->vte_count++;

	veriexec_report("New entry.", file, NULL, REPORT_DEBUG);

 out:
	vrele(nid.ni_vp);

	return (error);
}

/*
 * Create a new Veriexec table using hints from userland.
 *
 * Expects dict to have mount and count.
 */
int
veriexec_table_add(struct lwp *l, prop_dictionary_t dict)
{
	struct veriexec_table_entry *vte;
	struct nameidata nid;
	u_char buf[16];
	int error;

	NDINIT(&nid, LOOKUP, FOLLOW, UIO_SYSSPACE,
	    prop_string_cstring_nocopy(prop_dictionary_get(dict, "mount")), l);
	error = namei(&nid);
	if (error)
		return (error);

	error = fileassoc_table_add(nid.ni_vp->v_mount,
	    prop_number_integer_value(prop_dictionary_get(dict, "count")));
	if (error && (error != EEXIST))
		goto out;

	vte = malloc(sizeof(*vte), M_VERIEXEC, M_WAITOK | M_ZERO);
	error = fileassoc_tabledata_add(nid.ni_vp->v_mount, veriexec_hook, vte);
#ifdef DIAGNOSTIC
	if (error)
		panic("Fileassoc: Inconsistency after adding table");
#endif /* DIAGNOSTIC */

	snprintf(buf, sizeof(buf), "table%u", veriexec_tablecount++);
	sysctl_createv(NULL, 0, &veriexec_count_node, &vte->vte_node,
		       0, CTLTYPE_NODE, buf, NULL, NULL, 0, NULL,
		       0, CTL_CREATE, CTL_EOL);

	sysctl_createv(NULL, 0, &vte->vte_node, NULL,
		       CTLFLAG_READONLY, CTLTYPE_STRING, "mntpt",
		       NULL, NULL, 0, nid.ni_vp->v_mount->mnt_stat.f_mntonname,
		       0, CTL_CREATE, CTL_EOL);
	sysctl_createv(NULL, 0, &vte->vte_node, NULL,
		       CTLFLAG_READONLY, CTLTYPE_STRING, "fstype",
		       NULL, NULL, 0, nid.ni_vp->v_mount->mnt_stat.f_fstypename,
		       0, CTL_CREATE, CTL_EOL);
	sysctl_createv(NULL, 0, &vte->vte_node, NULL,
		       CTLFLAG_READONLY, CTLTYPE_QUAD, "nentries",
		       NULL, NULL, 0, &vte->vte_count, 0, CTL_CREATE, CTL_EOL);

 out:
	vrele(nid.ni_vp);
	return (error);
}

int
veriexec_table_delete(struct lwp *l, struct mount *mp) {
	struct veriexec_table_entry *vte;

	vte = veriexec_table_lookup(mp);
	if (vte == NULL)
		return (ENOENT);

	sysctl_free(__UNCONST(vte->vte_node));
	veriexec_tablecount--;

	return (fileassoc_table_clear(mp, veriexec_hook));
}

int
veriexec_file_delete(struct lwp *l, struct vnode *vp) {
	struct veriexec_table_entry *vte;
	int error;

	vte = veriexec_table_lookup(vp->v_mount);
	if (vte == NULL)
		return (ENOENT);

	error = fileassoc_clear(vp, veriexec_hook);
	if (!error)
		vte->vte_count--;

	return (error);
}

/*
 * Convert Veriexec entry data to a dictionary readable by userland tools.
 */
int
veriexec_convert(struct vnode *vp, prop_dictionary_t rdict)
{
	struct veriexec_file_entry *vfe;

	vfe = veriexec_get(vp);
	if (vfe == NULL)
		return (ENOENT);

	prop_dictionary_set_uint8(rdict, "entry-type", vfe->type);
	prop_dictionary_set_uint8(rdict, "status", vfe->status);
	prop_dictionary_set(rdict, "fp-type",
	    prop_string_create_cstring(vfe->ops->type));
	prop_dictionary_set(rdict, "fp",
	    prop_data_create_data(vfe->fp, vfe->ops->hash_len));

	return (0);
}

int
veriexec_unmountchk(struct mount *mp)
{
	int error;

	if (doing_shutdown)
		return (0);

	switch (veriexec_strict) {
	case VERIEXEC_LEARNING:
	case VERIEXEC_IDS:
		if (veriexec_table_delete(curlwp, mp) == 0) {
			log(LOG_INFO, "Veriexec: IDS mode, allowing  unmount "
			    "of \"%s\".\n", mp->mnt_stat.f_mntonname);
		}

		error = 0;
		break;

	case VERIEXEC_IPS: {
		struct veriexec_table_entry *vte;

		vte = fileassoc_tabledata_lookup(mp, veriexec_hook);
		if ((vte != NULL) && (vte->vte_count > 0)) {
			log(LOG_ALERT, "Veriexec: IPS mode, preventing"
			    " unmount of \"%s\" with monitored files.\n",
			    mp->mnt_stat.f_mntonname);

			error = EPERM;
		} else
			error = 0;
		break;
		}
 
	case VERIEXEC_LOCKDOWN:
	default:
		log(LOG_ALERT, "Veriexec: Lockdown mode, preventing unmount "
		    "of \"%s\".\n", mp->mnt_stat.f_mntonname);
		error = EPERM;
		break;
	}

	return (error);
}

int
veriexec_openchk(struct lwp *l, struct vnode *vp, const char *path, int fmode)
{
	boolean_t monitored = FALSE;
	int error = 0;

	if (vp == NULL) {
		/* If no creation requested, let this fail normally. */
		if (!(fmode & O_CREAT)) {
			error = 0;
			goto out;
		}

		/* Lockdown mode: Prevent creation of new files. */
		if (veriexec_strict >= VERIEXEC_LOCKDOWN) {
			log(LOG_ALERT, "Veriexec: Preventing new file "
			    "creation in `%s'.\n", path);
			error = EPERM;
		}

		goto out;
	}

	error = veriexec_verify(l, vp, path, VERIEXEC_FILE,
	    &monitored);
	if (error)
		goto out;

	if (monitored && ((fmode & FWRITE) || (fmode & O_TRUNC))) {
		veriexec_report("Write access request.", path, l,
		    REPORT_ALWAYS | REPORT_ALARM);

		/* IPS mode: Deny writing to/truncating monitored files. */
		if (veriexec_strict >= VERIEXEC_IPS)
			error = EPERM;
		else
			veriexec_purge(vp);
	}

 out:
	return (error);
}
