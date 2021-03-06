#ifndef __TABLES_H 
#define __TABLES_H 1

#include "symbol_tables.h"
#include "integer_stack.h"
#include "vectors.h"
#include "array_hash_map.h"
#include "hash_map.h"
#include "params.h"

/*
 * You should be able to set this to 0 to keep threads out of mcsat altogether:
 */
#define USE_PTHREADS 1

#define INIT_SORT_TABLE_SIZE 64
#define INIT_CONST_TABLE_SIZE 64
#define INIT_VAR_TABLE_SIZE 64
#define INIT_PRED_TABLE_SIZE 64
#define INIT_ATOM_PRED_SIZE 16
#define INIT_RULE_PRED_SIZE 16
#define INIT_ATOM_TABLE_SIZE 64
#define INIT_QUERY_TABLE_SIZE 16
#define INIT_QUERY_INSTANCE_TABLE_SIZE 64
#define INIT_SOURCE_TABLE_SIZE 16
/* vvv   Bug related to this?   vvv  */
#define INIT_RULE_INST_TABLE_SIZE 64
#define INIT_RULE_TABLE_SIZE 64
#define INIT_SORT_CONST_SIZE 16
#define INIT_CLAUSE_SIZE 16
#define INIT_RULE_SIZE 16
#define INIT_ATOM_SIZE 16
#define INIT_MODEL_TABLE_SIZE 64
#define INIT_SUBSTIT_TABLE_SIZE 8 /* number of vars in rules - likely to be small */

/*
 * MCSAT MAIN DATA STRUCTURES
 */

/* Truth values */
typedef enum {
	v_undef = -1, /* undefined */
	v_false = 0, /* false */
	v_true = 1, /* true */
	v_up_false = 2, /* false fixed by unit propagation */
	v_up_true = 3, /* true fixed by unit propagation */
	v_db_false = 4, /* false fixed by input db */
	v_db_true = 5 /* true fixed by input db */
} samp_truth_value_t;

/*
 * Boolean variables (atoms) are represented by 32bit integers.
 * For a variable x, the positive literal is 2x, the negative
 * literal is 2x + 1.
 *
 * -1 is a special marker for both variables and literals
 */
typedef int32_t samp_bvar_t;
typedef int32_t samp_literal_t;

enum {
	null_samp_bvar = -1,
	null_literal = -1,
};

/* Maximal number of boolean variables */
#define MAX_VARIABLES (INT32_MAX >> 1)

/* Conversions from variables to literals */
static inline samp_literal_t pos_lit(samp_bvar_t x) {
	return x<<1;
}

static inline samp_literal_t neg_lit(samp_bvar_t x) {
	return (x<<1) | 1;
}

/*
 * Makes a positive or negative literal from an atom
 *
 * mk_lit(x, 0) = pos_lit(x)
 * mk_lit(x, 1) = neg_lit(x)
 */
static inline samp_literal_t mk_lit(samp_bvar_t x, uint32_t sign) {
	assert((sign & ~1) == 0);
	return (x<<1)|sign;
}

/* Returns the atom of a literal */
static inline samp_bvar_t var_of(samp_literal_t l) {
	return l>>1;
}

/* true if l has positive polarity (i.e., l = pos_lit(x)) */
static inline bool is_pos(samp_literal_t l) {
	return !(l & 1);
}

/* true if l has negative polarity (i.e., l = neg_lit(x)) */
static inline bool is_neg(samp_literal_t l) {
	return (l & 1);
}

/* the same as is_neg, 0: pos; 1: neg */
static inline uint32_t sign_of_lit(samp_literal_t l) {
	return ((uint32_t) l) & 1;
}

/* negation of literal l */
static inline samp_literal_t not(samp_literal_t l) {
	return l ^ 1;
}

/* 
 * check whether l1 and l2 are opposite, i.e., same atom with
 * different signs
 */
static inline bool opposite(samp_literal_t l1, samp_literal_t l2) {
	return (l1 ^ l2) == 1;
}

/*
 * Each sort has a name (string), and a cardinality. Each element is also assigned
 * a number; the elements are prefixed with the sort.
 */
typedef struct sort_entry_s {
	int32_t size;
	int32_t cardinality; /* number of elements in sort i */
	char *name; /* print name of the sort */

	/* array of constants in the given sort, NULL when
	 * it is a integer sort */
	int32_t *constants;	
	/* array of integers for sparse integer sorts */
	int32_t *ints; 
	/* lower and upper bounds for integer sorts
	 * (could be a union type) */
	int32_t lower_bound; 
	int32_t upper_bound; 

	int32_t *subsorts; /* array of subsort indices */
	int32_t *supersorts; /* array of supersort indices */
} sort_entry_t;

/* List of sorts */
typedef struct sort_table_s {
	int32_t size;
	int32_t num_sorts; /* number of sorts */
	stbl_t sort_name_index; /* table giving index for sort name */
	sort_entry_t *entries; /* maps sort index to cardinality and name */
} sort_table_t;

typedef struct pred_entry_s {
	int32_t arity; /* number of arguments; negative for evidence predicate */
	int32_t *signature; /* pointer to an array of sort indices */
	char *name; /* print name */
	int32_t size_atoms;
	int32_t num_atoms;
	int32_t *atoms; /* This is atom indices, not atoms */
	/* Keep track of the rules that mention this predicate */
	int32_t size_rules;
	int32_t num_rules;
	int32_t *rules;
} pred_entry_t;

typedef struct pred_tbl_s {
	int32_t size;
	int32_t num_preds;
	pred_entry_t *entries;
} pred_tbl_t;

typedef struct pred_table_s  {
	pred_tbl_t evpred_tbl; /* signature map for evidence predicates */
	pred_tbl_t pred_tbl; /* signature map for non-evidence predicates */
	stbl_t pred_name_index; /* symbol table for all predicates */
} pred_table_t;

typedef  struct const_entry_s {
	char *name;
	uint32_t sort_index; 
} const_entry_t;

typedef struct const_table_s {
	int32_t size;
	int32_t num_consts;
	const_entry_t *entries; 
	stbl_t const_name_index; /* table mapping const name to index */
} const_table_t;

/* Quantified variables */
typedef struct var_entry_s {
	char *name;
	int32_t sort_index; 
} var_entry_t;

/* FIXME why do we keep them in a table? */
typedef struct var_table_s {
	int32_t size;
	int32_t num_vars;
	var_entry_t *entries; 
	stbl_t var_name_index; /* table mapping var name to index */
} var_table_t;

/*
 * An atom has a predicate symbol and an array of the corresponding arity.
 * Note that to get the arity, the pred_table must be available unless the
 * predicate is builtin.
 */
typedef struct samp_atom_s {
	int32_t pred; /* <= 0: direct pred; > 0: indirect pred */
	int32_t args[0];
} samp_atom_t;

typedef struct atom_table_s {
	int32_t size; /* size of the var_atom array (double for watched clause lists) */
	int32_t num_vars;  /* number of bvars */
	samp_atom_t **atom; /* the ground atom variables */
	/* Maps atoms to indices in the table. Uses the predicate index + indices
	 * of the arguments, an array with total length of arity + 1, as the key */
	array_hmap_t atom_var_hash; 

	bool *active; /* whether an atom is active */
	samp_truth_value_t *assignments[2]; /* maps atom ids to samp_truth_value_t */
	uint32_t assignment_index; /* which of two assignment arrays is current */
	samp_truth_value_t *assignment; /* the current assignment */
	int32_t num_unfixed_vars; /* number of unfixed vars */

	int32_t num_samples; /* current number of samples */
	int32_t *sampling_nums; /* number of samples BEFORE an atom is activated */
	int32_t *pmodel; /* model count of each atom */
} atom_table_t;

/* a clause is an array of literals. */
typedef struct samp_clause_s {
	//struct samp_clause_s *prev; /* ptr to the previous clause */
	//struct samp_clause_s *next; /* ptr to the next clause */
	struct samp_clause_s *link; /* ptr to the next clause */
	/* index of the rule that the clause belongs to, used for maxwalksat,
	 * ~rule_index is the (num_lits)th literal of the clause */
	int32_t rule_index;
	int32_t num_lits;
	samp_literal_t disjunct[0]; /* array of literals */
} samp_clause_t;

/* clause list */
typedef struct samp_clause_list_s {
	samp_clause_t *head;
	samp_clause_t *tail; /* pointer to the last element */
	int32_t length;
} samp_clause_list_t;

typedef struct rule_inst_s {
	double weight; /* weight of the clause: DBL_MAX for hard clause */
	int32_t num_clauses;
	samp_clause_t *conjunct[0]; /* array of clauses */
} rule_inst_t;

typedef struct rule_inst_table_s {
	int32_t size;   /* size of the rule_insts array */
	int32_t num_rule_insts; /* number of rule instance entries */
	rule_inst_t **rule_insts; /* array of rule instances */
	samp_truth_value_t *assignment; /* live or dead in mcsat, sat or unsat in mwsat */
	bool soft_rules_included; /* is false for the initial sample SAT */

	samp_clause_list_t *watched; /* maps literals to samp_clause pointers */
	samp_clause_list_t sat_clauses; /* list of fixed satisfied clauses */
	samp_clause_list_t unsat_clauses; /* list of unsat clauses, threaded through link */
	samp_clause_list_t live_clauses; /* temperory list to store unscaned live clauses */

	hmap_t unsat_soft_rules; /* unsat soft rule instances */
	double unsat_weight; /* total weight of unsat soft rules */
	double sat_weight;   /* total weight of sat soft rules */
	/* clauses of a soft rule that need not to be sat when the rule is unsat */
	samp_clause_list_t *rule_watched; 
} rule_inst_table_t;

/*
 * A rule is a clause with variables.  Instantiated forms of the rules are
 * added to the clause table.  The variables list is kept with each rule,
 * and the literal list is a list of actual literals, in order to reference
 * the variables.
 */
typedef enum {constant, variable, integer} arg_kind_t;

/*
 * In a rule, an argument may be:
 *  a constant (value is an index into the const_table)
 *  a variable (value is an index into the variable array of the associated rule)
 *  an integer (value is the integer)
 * When the rule is instantiated, a samp_atom_t is created, which only has constants
 * and integers - these are determined by the sorts.
 * We can't do this for rules, as this would not allow a distinction between
 * integers and variables.
 */
typedef struct rule_atom_arg_s {
	arg_kind_t kind;
	int32_t value;
} rule_atom_arg_t;

typedef struct rule_atom_s {
	int32_t pred;
	int32_t builtinop; /* = 0: not a built-in-op, > 0: built-in-op */
	rule_atom_arg_t *args;
} rule_atom_t;

typedef struct rule_literal_s {
	bool neg; /* true: with negation; false: without negation */
	bool grounded;
	rule_atom_t *atom;
} rule_literal_t;

/*
 * Substitution of all variables for a quantified clause.  INT32_MIN is used
 * to represent an unfixed variable.
 */
typedef int32_t substit_entry_t;
//typedef struct substit_entry_s {
//	int32_t const_index;
//	bool fixed;
//} substit_entry_t;

typedef struct rule_clause_s {
	int32_t num_lits;
	bool grounded;
	rule_literal_t *literals[0];
} rule_clause_t;

/* Quantified clause */
typedef struct samp_rule_s {
	array_hmap_t subst_hash; /* a hashset used to check duplicate of instances */
	//int32_t *clause_indices; /* array of indices into clause_table */
	//int32_t num_frozen; /* number of frozen predicates */
	//int32_t *frozen_preds; /* array of frozen predicates */
	double weight;
	int32_t num_vars; /* number of variables */
	var_entry_t **vars; /* The (quantified) variables */
	int32_t num_clauses; /* number of literal entries */
	rule_clause_t *clauses[0]; /* array of pointers to rule_literals */
} samp_rule_t;

typedef struct rule_table_s {
	int32_t size;   /* size of the samp_rule array */
	int32_t num_rules; /* number of rule entries */
	samp_rule_t **samp_rules; /* array of pointers to samp_rules */
} rule_table_t;

/* 
 * Keeps the uninstantiated form of the query, to be instantiated as new
 * constants come in. Similar to rules, but we can't separate the clauses, so
 * the literals array is one level deeper.
 *
 * TODO: Do not have to cnf-ize the query formulas, because we only need to
 * evaluate the formulas given an assignment. It could also support lazy
 * option, in which we examine the quantified formula w.r.t.  an assignment and
 * only instantiate the ground formulas that have a non-default value (Normally
 * the default value is false).
 */
typedef struct samp_query_s {
	int32_t *source_index; /* The source of this query, e.g., formula, learner id */
	int32_t num_clauses;
	int32_t num_vars;
	var_entry_t **vars;
	rule_literal_t ***literals;
} samp_query_t;

typedef struct query_table_s {
	int32_t size; /* size of the query array */
	int32_t num_queries;
	samp_query_t **query;
} query_table_t;

/*
 * Keeps the instantiated form, along with the sampling numbers and pmodels as
 * for the atom_table_t.
 */
typedef struct samp_query_instance_s {
	// int32_t query_index; /* Index to the query from which this was generated */
	ivector_t query_indices; /* Indices to the queries from which this was generated */
	int32_t sampling_num; /* The num_samples when this instance was created */
	int32_t pmodel; /* The nimber of samples for which this instance was true */
	int32_t *subst; /* Holds the mapping from vars to consts */
	bool *constp; /* Whether given var is a constant or integer */
	samp_literal_t **lit; /* The instance - a conjunction of disjunctions of lits */
} samp_query_instance_t;

typedef struct query_instance_table_s {
	int32_t size;
	int32_t num_queries;
	samp_query_instance_t **query_inst;
} query_instance_table_t;

/*
 * Sources indicate the source of the assertion, rule, etc.
 * and is used to reset, untell, etc.  For each explicitly
 * provided source, we keep track of the formula and assertion
 * So we can undo the effects.  For weighted formulas, we keep
 * track of the weight so we can subtract it later.
 */
typedef struct source_entry_s {
	char *name;
	int32_t *assertion; /* indices to atom_table, -1 terminated */
	int32_t *rule_insts; /* indices to rule_inst_table; -1 terminated */
	double *weight; /* weights corresponding to clause list */
} source_entry_t;

typedef struct source_table_s {
	int32_t size;
	int32_t num_entries;
	source_entry_t **entry;
} source_table_t;

/* All the tables */
typedef struct samp_table_s {
  sort_table_t sort_table;
  const_table_t const_table;
  var_table_t var_table;
  pred_table_t pred_table;
  atom_table_t atom_table;
  rule_table_t rule_table; /* formulas with variables */
  rule_inst_table_t rule_inst_table;
  query_table_t query_table;
  query_instance_table_t query_instance_table;
  source_table_t source_table;
  /* Purely ephemeral, but allocated per samp_table object for thread
   * safety:
   */
  integer_stack_t clause_var_stack;
} samp_table_t;


/* functions for sort_table */
extern void init_sort_table(sort_table_t *sort_table);
extern void reset_sort_table(sort_table_t *sort_table);
extern void add_sort(sort_table_t *sort_table, char *name);
extern void add_subsort(sort_table_t *sort_table, char *subsort, char *supersort);
extern int32_t sort_name_index(char *name, sort_table_t *sort_table);
extern int32_t *sort_signature(char **in_signature, int32_t arity, sort_table_t *sort_table);
extern int32_t get_sort_const(sort_entry_t *sort_entry, int32_t index);

/* functions for const_table */
extern void init_const_table(const_table_t *const_table);
extern int32_t const_index(char *name, const_table_t *const_table);
extern int32_t const_sort_index(int32_t const_index, const_table_t *const_table);
extern void sort_entry_resize(sort_entry_t *sort_entry);
extern void add_const_to_sort(int32_t const_index,
		int32_t sort_index,
		sort_table_t *sort_table);
extern int32_t add_const_internal (char *name, int32_t sort_index,
		samp_table_t *table);
extern int32_t add_const(char *name, char * sort_name, samp_table_t *table);
extern char *const_name(int32_t const_index, const_table_t *const_table);

/* functions for var table */
extern int32_t var_index(char *name, var_table_t *var_table);
extern int32_t add_var(var_table_t *var_table,
		char *name,
		sort_table_t *sort_table,
		char * sort_name);

/* functions for pred_table */
extern void init_pred_table(pred_table_t *pred_table);
extern int32_t add_pred(pred_table_t *pred_table, char *name, bool evidence,
		int32_t arity, sort_table_t *sort_table, char **in_signature);

extern void pred_atom_table_resize(pred_entry_t *pred_entry);
extern void add_atom_to_pred(pred_table_t *pred_table, int32_t predicate,
		int32_t current_atom_index);

extern void pred_rule_table_resize(pred_entry_t *pred_entry);
extern void add_rule_to_pred(pred_table_t *pred_table,
		int32_t predicate,
		int32_t current_rule_index);

extern bool pred_epred(int32_t predicate);
extern int32_t pred_val_to_index(int32_t val);
extern bool atom_eatom(int32_t atom_id, pred_table_t *pred_table, atom_table_t *atom_table);
extern pred_entry_t *get_pred_entry(pred_table_t *pred_table, int32_t predicate);
extern int32_t pred_arity(int32_t predicate, pred_table_t *pred_table);
extern int32_t pred_index(char * in_predicate, pred_table_t *pred_table);
extern char *pred_name(int32_t pred, pred_table_t *pred_table);
extern int32_t pred_default_value(pred_entry_t *pred);
extern int32_t *pred_signature(int32_t predicate, pred_table_t *pred_table);

/* functions for atom_table */
extern void init_atom_table(atom_table_t *table);
extern void atom_table_resize(atom_table_t *atom_table, rule_inst_table_t *rule_inst_table);

/* functions for rule_table */
extern void init_rule_table(rule_table_t *table);
extern void rule_table_resize(rule_table_t *rule_table);

/* functions for rule_inst_table */
extern void init_rule_inst_table(rule_inst_table_t *table);
extern void rule_inst_table_resize(rule_inst_table_t *rule_inst_table);

/* functions for query_table */
extern void init_query_table(query_table_t *table);
extern void query_table_resize(query_table_t *table);

/* functions for query_instance_table */
extern void init_query_instance_table(query_instance_table_t *table);
extern void query_instance_table_resize(query_instance_table_t *table);
extern void reset_query_instance_table(query_instance_table_t *table);

/* functions for source_table */
extern void init_source_table(source_table_t *table);
extern void source_table_extend(source_table_t *table);
extern void reset_source_table(source_table_t *table);

extern void add_source_to_clause(char *source, int32_t clause_index, double weight,
		samp_table_t *table);
extern void add_source_to_assertion(char *source, int32_t atom_index,
		samp_table_t *table);
extern void retract_source(char *source, samp_table_t *table);

/* functions for samp_table (the struct of all table) */
extern void init_samp_table(samp_table_t *table);
extern bool valid_table(samp_table_t *table);

extern samp_table_t *clone_samp_table(samp_table_t *table);
extern void merge_atom_tables(atom_table_t *to, atom_table_t *from);
extern void merge_query_instance_tables(query_instance_table_t *to, query_instance_table_t *from);

#endif /* __TABLES_H */
