/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "threads/vaddr.h"
#include "userprog/process.h"

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	/* spt 에 할당 안되어 있음! 여기서 upage는 가상 주소 공간이고, 여기다 페이지 할당 받고 싶은거임*/
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		
		struct page *page = (struct page *)malloc(sizeof(struct page));

		if(VM_TYPE(type) == VM_ANON){
			uninit_new(page, upage, init, type, aux, anon_initializer);
		}else if (VM_TYPE(type) == VM_FILE){
			uninit_new(page, upage, init, type, aux, file_backed_initializer);
		}

		page->writable = writable;
		/* TODO: Insert the page into the spt. */
		return spt_insert_page(spt, page);
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page ( struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function. */
	page = (struct page *)malloc(sizeof(struct page));
	struct hash_elem *e;

	// va에 해당하는 hash_elem 찾기
	page->va = pg_round_down(va); // page의 시작 주소 할당
	e = hash_find(&spt->spt_hash_table, &page->hash_elem);
	free(page);
	
	// 있으면 e에 해당하는 페이지 반환
	return e != NULL ? hash_entry(e, struct page, hash_elem) : NULL;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	// int succ = false;
	// /* TODO: Fill this function. */
	
	// struct hesh_elem *check = hash_insert(&spt->spt_hash_table, &page->hash_elem);
	// if(check == NULL){
	// 	succ = true;
	// }
	// return succ;
	return hash_insert(&spt->spt_hash_table, &page->hash_elem) == NULL ? true : false;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) { 
	/* TODO: Fill this function. */
	struct frame *frame = (struct frame *)malloc(sizeof(struct frame));
	frame->kva = palloc_get_page(PAL_USER);
	
	if (frame->kva == NULL) {
		PANIC("todo: later check! minji - no space for it");
		/* 나중에 물리 메모리 공간이 없을 경우, swap!! 해야함.*/
	}

	frame->page = NULL;
	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
	vm_alloc_page(VM_ANON | VM_MARKER_0, pg_round_down(addr), 1);
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
	bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;

	if(not_present){
		uintptr_t *current_rsp = f->rsp;
		/*만약 커널모드에서 rsp 를 불러오면, 커널 스택포인터를 가리키기 때문에, 현재 thread에서 가져오는게 맞음*/
		if(!user){
			current_rsp = thread_current()->tf.rsp;
		}
		//USER_STACK - (1 << 20) = 스택 최대 크기 = 1MB
		if (USER_STACK - (1 << 20) <= current_rsp - 8 && current_rsp-8 <= addr && addr <= USER_STACK){
			vm_stack_growth(addr);
		}
	 	page = spt_find_page(spt, addr);
		if(page == NULL) {
			return false;
		}
		if (write && (!page->writable)) { //권한이 없는데 쓰려고 하는 경우
			return false;
		}
		return vm_do_claim_page(page);
	}
	return false;
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function */
	/* va 를 가지고 페이지를 가져오라는거겠지??*/
	struct thread *curr = thread_current ();

	page = spt_find_page(&curr->spt, va);
	if(page == NULL){
		return NULL;
	}
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();
	struct thread *curr = thread_current ();
	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	/* 이미 vm_get_frame 에서 페이지 할당 못받으면 패닉 시키니까 여기서 따로 처리 안함
	우선 무조건 반환 받았다고 가정 */

	bool check = pml4_set_page(curr->pml4, page->va, frame->kva, page->writable);
	if(!check) {
		return false;
	}
	return swap_in (page, frame->kva);
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(&spt->spt_hash_table, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
/* 깊은 복사가 이루어져야 해서, 각각의 데이터를 다 하나씩 복사해줘야 함*/
/* 타입에 따라, 초기화가 다르다는 것을 기억 해야함 */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
	struct supplemental_page_table *src UNUSED) {
		struct hash_iterator i;
		hash_first(&i, &src->spt_hash_table);
		while(hash_next(&i)){
			/* 페이지 찾고 */
			struct page *src_page = hash_entry(hash_cur(&i), struct page, hash_elem);
			enum vm_type type = src_page->operations->type;
			void *upage = src_page->va;
			bool writable = src_page->writable;
			enum vm_type real_type = page_get_type(src_page);
			/* 만약에 type 이 VM_UNINIT 이면, 초기화 해줘야함. */

			if(type == VM_UNINIT) {
				vm_initializer *init = src_page->uninit.init;
				void *aux = src_page->uninit.aux;
				vm_alloc_page_with_initializer(real_type, upage, writable, init, aux);
				continue;
			}
			if (type == VM_FILE){
				struct lazy_load_arg *arg = malloc(sizeof(struct lazy_load_arg));
				arg->file = src_page->file.file;
				arg->ofs = src_page->file.ofs;
				arg->read_bytes = src_page->file.read_bytes;
				arg->zero_bytes = src_page->file.zero_bytes;
				if (!vm_alloc_page_with_initializer(type, upage, writable, NULL, arg))
					return false;
				struct page *file_page = spt_find_page(dst, upage);
				file_backed_initializer(file_page, type, NULL);
				file_page->frame = src_page->frame;
				pml4_set_page(thread_current()->pml4, file_page->va, src_page->frame->kva, src_page->writable);
				continue;
			}
				/* 3) type이 anon이면 */
			if (!vm_alloc_page(type, upage, writable)) // uninit page 생성 & 초기화
				return false;						   // init이랑 aux는 Lazy Loading에 필요. 지금 만드는 페이지는 기다리지 않고 바로 내용을 넣어줄 것이므로 필요 없음

			// vm_claim_page으로 요청해서 매핑 & 페이지 타입에 맞게 초기화
			if (!vm_claim_page(upage))
				return false;

			// 매핑된 프레임에 내용 로딩
			struct page *dst_page = spt_find_page(dst, upage);
			memcpy(dst_page->frame->kva, src_page->frame->kva, PGSIZE);
			}
		return true;
	}
/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	hash_clear(&spt->spt_hash_table, hash_page_destroy); // 해시 테이블의 모든 요소를 제거
}
void hash_page_destroy(struct hash_elem *e, void *aux)
{
	struct page *page = hash_entry(e, struct page, hash_elem);
	destroy(page);
	free(page);
}
/* Returns a hash value for page p. */
unsigned
page_hash (const struct hash_elem *p_, void *aux UNUSED) {
  const struct page *p = hash_entry (p_, struct page, hash_elem);
  return hash_bytes (&p->va, sizeof p->va);
}
/* Returns true if page a precedes page b. */
bool
page_less (const struct hash_elem *a_,
           const struct hash_elem *b_, void *aux UNUSED) {
  const struct page *a = hash_entry (a_, struct page, hash_elem);
  const struct page *b = hash_entry (b_, struct page, hash_elem);

  return a->va < b->va;
}