#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct ricetta {
    char *nome; //key
    unsigned short peso;
    char **ingrediente; // array of pointers
    unsigned int *quantita; // each int is assosiated with an ingredient
    unsigned int total_ingredients;
} ricetta;

typedef struct ric_node {
    ricetta *ricetta;
} ric_node;

typedef struct ric_node_arr {
    ricetta **array;
    unsigned int size;
    unsigned int capacity;
} ric_node_arr;

////////////////////
typedef struct ordine {
    unsigned int time_added; //primary key. id
    ricetta *ricetta; //a pointer
    unsigned int quantita;
    unsigned short peso; //sum of the weight of each receipt per corrispondent quantity. initially 0.

    char *missing_ing;
    unsigned int missing_q;
} ordine;

typedef struct ordine_node {
    ordine *ord;
    struct ordine_node *next;
} ordine_node;

typedef struct {
    ordine **orders;
    unsigned int size;
    unsigned capacity;
} ordini_arr;

////////////////////
typedef struct lotto {
    unsigned int scadenza;
    unsigned int quantita;
    struct lotto *next;
} lotto;

typedef struct lotto_node {
    char *ingredient; //key
    lotto *head; //value
    struct lotto_node *next; // for collisions
} lotto_node;

typedef struct {
    ric_node_arr **table;
    unsigned int size;
    unsigned int capacity;
} rec_hash_table;

typedef struct {
    lotto_node **table;
    unsigned int size; //to delete
    unsigned int capacity;

    unsigned int *occupied;
    unsigned int occupied_dim;
} lotto_hash_table;

unsigned int t = 0; //tempo attuale
unsigned int periodo = 0; // corriere passa ogni tot
unsigned int peso_max = 0; // peso massimo che il corriere puo' portare

rec_hash_table *ricette = NULL; //all the receipts that are present

ordini_arr *orders_pronti = NULL;
ordini_arr *orders_attesa = NULL;
/**
* hash table with the key as ingredient and value as an ordered linked list
*/
lotto_hash_table *magazzino = NULL;

char input_word[21] = ""; //request word

void insert_receipt_in_table(ricetta *receipt);

void delete_order(ordini_arr *list, const unsigned int time_add);

///////////////////////////////////////////////////////////////////////
unsigned int hash(const char *str, int capacity) {
    //found, not mine. hash needs a good function
    unsigned int hash = 6151; //prime
    unsigned int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash % capacity;
}

void insert_ricetta_in_arr(ric_node_arr *r_array, ricetta *new_ricetta) {
    // lexicographical order
    unsigned int low = 0;
    unsigned int high = r_array->size;
    unsigned int mid;

    while (low < high) {
        mid = (low + high) / 2;
        if (strcmp(r_array->array[mid]->nome, new_ricetta->nome) < 0) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }
    // resize the array if it's full
    if (r_array->size == r_array->capacity) {
        r_array->capacity *= 2;
        r_array->array = (ricetta **) realloc(r_array->array, r_array->capacity * sizeof(ricetta *));
    }

    // shift
    for (unsigned int j = r_array->size; j > low; j--) {
        r_array->array[j] = r_array->array[j - 1];
    }

    r_array->array[low] = new_ricetta;
    r_array->size++;
}

rec_hash_table *create_rec_hash_table(const unsigned int old_size, const unsigned int capacity) {
    rec_hash_table *table = (rec_hash_table *) malloc(sizeof(rec_hash_table));

    table->size = old_size;
    table->capacity = capacity;

    table->table = (ric_node_arr **) calloc(capacity, sizeof(ric_node_arr *));

    for (unsigned int i = 0; i < capacity; i++) {
        table->table[i] = NULL;
    }

    if (old_size > 0) {
        for (unsigned int i = 0; i < capacity; i++) {
            ric_node_arr *old_array = ricette->table[i];
            for (unsigned int j = 0; j < old_array->size; j++) {
                ricetta *ric = old_array->array[j];
                unsigned int new_index = hash(ric->nome, capacity);

                if (table->table[new_index] == NULL) {
                    table->table[new_index] = (ric_node_arr *) malloc(sizeof(ric_node_arr));
                    table->table[new_index]->array = (ricetta **) malloc(sizeof(ricetta *));
                    table->table[new_index]->size = 0;
                    table->table[new_index]->capacity = 1;
                    table->size++; // Increase the count for the new table
                }

                insert_ricetta_in_arr(table->table[new_index], ric);
            }

            free(old_array->array);
            free(old_array);
        }
        // free the old hash table's array and structure itself
        free(ricette->table);
        free(ricette);
    }

    return table;
}

lotto_hash_table *create_hash_table(const unsigned int old_size, const unsigned int capacity) {
    lotto_hash_table *table = (lotto_hash_table *) malloc(sizeof(lotto_hash_table));
    table->table = (lotto_node **) malloc(capacity * sizeof(lotto_node *));
    for (unsigned int i = 0; i < capacity; i++) {
        table->table[i] = NULL;
    }
    table->size = old_size;
    table->capacity = capacity;
    table->occupied_dim = 0;
    unsigned int tmp = capacity * 0.75;
    table->occupied = (unsigned int *) malloc(tmp * sizeof(unsigned int));
    for (unsigned int i = 0; i < tmp; i++) {
        table->occupied[i] = 0; // or 0, depending on the use case
    }
    return table;
}

///////////////////////////////////////////////////////////////////////
/**
* allocate space for a new rec.
* calls    insert_receipt_in_table(new_ric);
*/
void insert_receipt(const char *nome, char **ing, const int *quantita, int ing_count) {
    ricetta *new_ric = (ricetta *) malloc(sizeof(ricetta));
    new_ric->nome = (char *) malloc((strlen(nome) + 1) * sizeof(char)); //do i need to add 1?
    strcpy(new_ric->nome, nome);
    new_ric->ingrediente = (char **) malloc(ing_count * sizeof(char *));
    new_ric->quantita = (unsigned int *) malloc(ing_count * sizeof(unsigned int));
    new_ric->peso = 0;
    for (int i = 0; i < ing_count; i++) {
        new_ric->ingrediente[i] = strdup(ing[i]);
        new_ric->quantita[i] = quantita[i];
        new_ric->peso += quantita[i];
    }
    new_ric->total_ingredients = ing_count;
    insert_receipt_in_table(new_ric);
}

///////////////////////////////////////////////
void rebalance_rec() {
    const unsigned int old_capacity = ricette->capacity;
    const unsigned int new_capacity = (unsigned int) (old_capacity * 2);

    rec_hash_table *new_table = create_rec_hash_table(0, new_capacity);

    for (unsigned int i = 0; i < old_capacity; i++) {
        ric_node_arr *r_array = ricette->table[i];
        if (r_array != NULL) {
            for (unsigned int j = 0; j < r_array->size; j++) {
                ricetta *ric = r_array->array[j];
                const unsigned int new_index = hash(ric->nome, new_capacity);

                if (new_table->table[new_index] == NULL) {
                    new_table->table[new_index] = (ric_node_arr *) malloc(sizeof(ric_node_arr));
                    new_table->table[new_index]->array = (ricetta **) malloc(sizeof(ricetta *));
                    new_table->table[new_index]->size = 0;
                    new_table->table[new_index]->capacity = 1;
                }

                insert_ricetta_in_arr(new_table->table[new_index], ric);
            }
            free(r_array->array);
            free(r_array);
        }
    }

    free(ricette->table);
    ricette->table = new_table->table;
    ricette->capacity = new_capacity;
    free(new_table);
}

void rebalance_lotto() {
    //for expanding the memory for ingredients
    unsigned int old_capacity = magazzino->capacity;
    unsigned int new_capacity = old_capacity * 1.7;
    unsigned int new_dim = 0;
    lotto_hash_table *new_table = create_hash_table(magazzino->size, new_capacity);

    for (unsigned int i = 0; i < old_capacity; i++) {
        lotto_node *node = magazzino->table[i];
        while (node != NULL) {
            lotto_node *next_node = node->next;
            unsigned int new_index = hash(node->ingredient, new_capacity);

            if (new_table->table[new_index] == NULL) {
                new_dim++;
                magazzino->occupied[new_dim] = new_index;
            }
            node->next = new_table->table[new_index];
            new_table->table[new_index] = node;


            node = next_node;
        }
    }

    free(magazzino->table);
    magazzino->table = new_table->table;
    magazzino->capacity = new_capacity;
    magazzino->occupied_dim = new_dim;
    free(new_table);
}

///////////////////////////////////////////////////////////////////////
void insert_lotto(const char *ingredient, unsigned int time_scadenza, int quantity) {
    if (magazzino->size > 0.8 * magazzino->capacity) {
        rebalance_lotto();
    }

    unsigned int index = hash(ingredient, magazzino->capacity);

    lotto_node *node = magazzino->table[index];
    lotto_node *prev_node = NULL;

    bool is_new = (node == NULL);
    while (node != NULL && strcmp(node->ingredient, ingredient) < 0) {
        //untill ing don't finish or I find it in the warehouse
        node = node->next;
        prev_node = node;
    }

    if (node == NULL || strcmp(node->ingredient, ingredient) != 0) {
        //if not yet present, create such an ingredient
        lotto_node *new_node = (lotto_node *) malloc(sizeof(lotto_node));
        new_node->ingredient = strdup(ingredient); //cse sencetive
        new_node->head = NULL;
        new_node->next = node;

        node = new_node;
        if (prev_node == NULL) {
            magazzino->table[index] = new_node;
        } else {
            prev_node->next = new_node;
        }
        magazzino->size++;

        if (is_new) {
            magazzino->occupied_dim++;
            magazzino->occupied[magazzino->occupied_dim] = index;
        }
    }

    lotto *current = node->head;
    lotto *lotto_prev = NULL;

    lotto *temp = node->head;
    while (temp != NULL) {
        temp = temp->next;
    }

    while (current != NULL && current->scadenza < time_scadenza) {
        lotto_prev = current;
        current = current->next;
    }

    if (current != NULL && current->scadenza == time_scadenza) {
        // found  a node with the same scadenza
        current->quantita += quantity;
    } else {
        // new node
        lotto *new_lotto = (lotto *) malloc(sizeof(lotto));
        new_lotto->scadenza = time_scadenza;
        new_lotto->quantita = quantity;
        new_lotto->next = current;

        if (lotto_prev == NULL) {
            //first one
            node->head = new_lotto;
        } else {
            lotto_prev->next = new_lotto;
        }
    }
}

void buttare_scaduto() {
    for (unsigned int i = 0; i < magazzino->occupied_dim; i++) {
        unsigned int index = magazzino->occupied[i];
        lotto_node *node = magazzino->table[index];
        lotto_node *prev_node = NULL;

        while (node != NULL) {
            lotto *current = node->head;

            //delete ones with scadenza < t
            while (current != NULL && current->scadenza <= t) {
                node->head = current->next;
                free(current);
                current = node->head;
            }

            if (node->head == NULL) {
                // node is empty, free it
                if (prev_node == NULL) {
                    magazzino->table[index] = node->next;
                } else {
                    prev_node->next = node->next;
                }

                lotto_node *temp = node;
                node = node->next;
                free(temp->next);
                free(temp->head);
                free(temp->ingredient);
                free(temp);
                magazzino->size--;
            } else {
                break;
            }
        }
    }
}

void consume_ingredient(const char *ing, unsigned int amount) {
    unsigned int index = hash(ing, magazzino->capacity);

    if (index >= magazzino->capacity || magazzino->table[index] == NULL) {
        return;
    }
    lotto_node *node = magazzino->table[index];
    lotto_node *prev_node = NULL;
    // looking for the ingredient until a match
    while (node != NULL && strcmp(node->ingredient, ing) < 0) {
        prev_node = node;
        node = node->next;
    }

    lotto *current = node->head;
    while (current != NULL) {
        if (current->quantita <= amount) {
            //need more then one node
            // subtract the whole quantity and delete the node
            amount -= current->quantita;
            node->head = current->next;
            free(current);
            current = node->head;

            if (node->head == NULL) {
                //it was the last lotto
                if (prev_node == NULL) {
                    magazzino->table[index] = node->next;
                } else {
                    prev_node->next = node->next;
                }
                free(node->ingredient);
                free(node);
                magazzino->size--;
                return;
            }
        } else {
            //less then one node
            // subtract part of the quantity
            current->quantita -= amount;
            break;
        }
    }
}

///////////////////////////
/**
* if the node exists, then       insert_rec_in_node(current, receipt);
* if no, then allocates space for the node and inserts it
*/
void insert_receipt_in_table(ricetta *receipt) {
    if (ricette->size > 0.8 * ricette->capacity) {
        rebalance_rec();
    }

    const unsigned int index = hash(receipt->nome, ricette->capacity);
    if (ricette->table[index] == NULL) {
        ricette->table[index] = (ric_node_arr *) malloc(sizeof(ric_node_arr));
        ricette->table[index]->array = (ricetta **) malloc(sizeof(ricetta *));
        ricette->table[index]->size = 0;
        ricette->table[index]->capacity = 1;
        ricette->size++;
    }

    insert_ricetta_in_arr(ricette->table[index], receipt);
}

/**
* retunrs NULL, if not found
*/
ricetta *search_receipt(const char *ric) {
    if (ricette->size == 0) {
        return NULL;
    }
    const unsigned int ind = hash(ric, ricette->capacity);

    if (ricette->table[ind] == NULL) {
        return NULL;
    }

    ric_node_arr *r_array = ricette->table[ind];

    unsigned int low = 0;
    unsigned int high = r_array->size;

    while (low < high) {
        unsigned int mid = (low + high) / 2;
        int cmp = strcmp(r_array->array[mid]->nome, ric);

        if (cmp == 0) {
            return r_array->array[mid]; // found
        }
        if (cmp < 0) {
            low = mid + 1; // the right half
        } else {
            high = mid; //  the left half
        }

    }
    return NULL;
}

void aggiunere_ricetta() {
    char nome[21];

    if (scanf("%s", nome) != 1) {
        printf("error. reading receipe name.\n");
        return;
    }

    if (search_receipt(nome) != NULL) {
        // if already present
        char *buffer = NULL;
        size_t b_size = 0;
        ssize_t buttare = getline(&buffer, &b_size, stdin);

        if (buttare == -1) {
            printf("error. povna dupa \n");
        }
        int c = getchar();
        if (c != '\n' || c != EOF) {
            ungetc(c, stdin); // put the character back if it's not a newline
        }
        printf("ignorato\n");
        return;
    }

    char nome_ingrediente[21];
    unsigned int quantita = 0;
    char **ing = NULL;
    int *quantita_arr = NULL;
    unsigned int ing_count = 0;

    while (scanf("%s %u", nome_ingrediente, &quantita) == 2) {
        ing_count++;
        ing = (char **) realloc(ing, ing_count * sizeof(char *));
        quantita_arr = (int *) realloc(quantita_arr, ing_count * sizeof(int));
        ing[ing_count - 1] = strdup(nome_ingrediente);
        quantita_arr[ing_count - 1] = quantita;
        char c = getchar();
        if (c == '\n' || c == EOF) {
            break;
        }

        ungetc(c, stdin); // put the character back if it's not a newline
    }
    insert_receipt(nome, ing, quantita_arr, ing_count); // compose the new receipt using input
    printf("aggiunta\n");
    free(ing);
    free(quantita_arr);
}

void free_ricetta(ricetta *r) {
    if (r == NULL) {
        return;
    }

    if (r->ingrediente != NULL) {
        for (unsigned int i = 0; i < r->total_ingredients; i++) {
            if (r->ingrediente[i] != NULL) {
                free(r->ingrediente[i]);
            }
        }
        free(r->ingrediente);
    }
    free(r->quantita);
    free(r->nome);
    free(r);
}

void delete_receipt(const char *ric) {
    unsigned int index = hash(ric, ricette->capacity);
    ric_node_arr *r_array = ricette->table[index];

    if (r_array == NULL || r_array->size == 0) {
        //check the condition
        printf("non presente\n");
        return;
    }

    unsigned int low = 0;
    unsigned int high = r_array->size;
    unsigned int mid = 0;
    int cmp = -1;

    while (low < high) {
        mid = (low + high) / 2;
        cmp = strcmp(r_array->array[mid]->nome, ric);

        if (cmp == 0) {
            break; // found
        }

        if (cmp < 0) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }

    if (low >= high || cmp != 0) {
        printf("non presente\n");
        return; //  not found
    }

    free(r_array->array[mid]);

    // shift elements to fill the gap
    for (unsigned int j = mid; j < r_array->size - 1; j++) {
        r_array->array[j] = r_array->array[j + 1];
    }

    r_array->size--;
    ricette->size--;

    printf("rimossa\n");
}

bool search_if_ordered(const char *nome) {
    //true if a recipet has been ordered and not yet delivered
    for (unsigned int i = 0; i < orders_attesa->size; i++) {
        if (strcmp(orders_attesa->orders[i]->ricetta->nome, nome) == 0) {
            return true;
        }
    }

    for (unsigned int i = 0; i < orders_pronti->size; i++) {
        if (strcmp(orders_pronti->orders[i]->ricetta->nome, nome) == 0) {
            return true;
        }
    }

    return false;
}

void rimuovere_ricetta() {
    char nome[21];
    if (scanf("%s", nome) != 1) {
        //should never happen
        printf("test. error reading recipe name\n");
        return;
    }

    char c = getchar(); //I need to get rid of the newline character
    if (c == '\n') {
        //should be like this
        c = getchar();

        if (c != EOF && c != '\n') {
            ungetc(c, stdin);
        }
    } else if (c != EOF) {
        ungetc(c, stdin);
    }

    //serch in the list of current orders
    if (search_if_ordered(nome)) {
        printf("ordini in sospeso\n");
        return;
    }
    delete_receipt(nome);
}

///////////////////////////////////////////////////////////////////////
ordine *create_ordine(int quantita, ricetta *ric) {
    ordine *ord = (ordine *) malloc(sizeof(ordine));
    ord->time_added = t;
    ord->quantita = quantita;
    ord->ricetta = ric;
    ord->peso = quantita * ric->peso; // total weight

    return ord;
}

void free_ordine(ordine *ord) {
    if (ord == NULL) {
        return;
    }
    free(ord);
}

ordine_node *create_order_node(ricetta *ric, int quantita) {
    ordine *new_order = create_ordine(quantita, ric);
    ordine_node *new_node = (ordine_node *) malloc(sizeof(ordine_node));
    new_node->ord = new_order;
    new_node->next = NULL;
    new_node->ord->missing_ing = NULL;
    new_node->ord->missing_q = 0;

    return new_node;
}

ordini_arr *init_orders(unsigned int cap) {
    ordini_arr *orders = (ordini_arr *) malloc(sizeof(ordini_arr));
    orders->orders = (ordine **) malloc(cap * sizeof(ordine *));
    orders->size = 0;
    orders->capacity = cap;
    return orders;
}

void move_order(ordine *order) {
    //from attesa to pronto only
    unsigned int index = 0;
    while (index < orders_attesa->size && orders_attesa->orders[index] != order) {
        index++;
    }

    // Shift elements to fill the gap in ordini_attesa
    for (unsigned int i = index; i < orders_attesa->size - 1; i++) {
        orders_attesa->orders[i] = orders_attesa->orders[i + 1];
    }
    orders_attesa->size--;

    if (orders_pronti->size == orders_pronti->capacity) {
        orders_pronti->capacity *= 2;
        orders_pronti->orders = (ordine **) realloc(orders_pronti->orders, orders_pronti->capacity * sizeof(ordine *));
    }

    unsigned int insert_index = 0;
    while (insert_index < orders_pronti->size && orders_pronti->orders[insert_index]->time_added < order->time_added) {
        insert_index++;
    }

    // Shift elements in ordini_pronti to make room for the new order
    for (unsigned int i = orders_pronti->size; i > insert_index; i--) {
        orders_pronti->orders[i] = orders_pronti->orders[i - 1];
    }

    orders_pronti->orders[insert_index] = order;
    orders_pronti->size++;
}

void delete_order(ordini_arr *list, const unsigned int time_add) {
    //for complete deletion
    unsigned int index = 0;

    // find the order with time_added
    while (index < list->size && list->orders[index]->time_added != time_add) {
        index++;
    }

    if (list->orders[index] != NULL) {
        free_ordine(list->orders[index]);
    }

    // fill the gap
    for (unsigned int i = index; i < list->size - 1; i++) {
        list->orders[i] = list->orders[i + 1];
    }
    list->size--;
}

///////////////////////////////////////////////////////////////////////
void free_receipt_list() {
    // to free space. EOB
    if (ricette) {
        for (unsigned int i = 0; i < ricette->capacity; i++) {
            ric_node_arr *r_array = ricette->table[i];

            if (r_array != NULL) {
                for (unsigned int j = 0; j < r_array->size; j++) {
                    free_ricetta(r_array->array[j]); // free each ricetta
                }
                free(r_array->array);
                free(r_array);
            }
        }
        free(ricette->table);
        free(ricette);
        ricette = NULL;
    }
}

void free_hash_magazzino() {
    for (unsigned int i = 0; i < magazzino->capacity; i++) {
        lotto_node *node = magazzino->table[i];
        while (node != NULL) {
            lotto_node *next = node->next;
            lotto *current = node->head;
            while (current != NULL) {
                lotto *next_lotto = current->next;
                free(current);
                current = next_lotto;
            }
            free(node->ingredient);
            free(node);
            node = next;
        }
    }
    free(magazzino->table);
    free(magazzino);
}

////////////////////////////////////////////////////////////////////////
bool one_available(const char *ing, const unsigned int qty) {
    unsigned int index = hash(ing, magazzino->capacity);

    lotto_node *node = magazzino->table[index];

    while (node != NULL && strcmp(node->ingredient, ing) != 0) {
        node = node->next;
    }

    if (node == NULL) {
        return false; // Ingredient not found
    }

    lotto *current = node->head;
    unsigned int total_quantity = 0;

    while (current != NULL) {
        total_quantity += current->quantita;

        if (total_quantity >= qty) {
            // There is enough of the ingredient
            break;
        }
        current = current->next;
    }

    if (total_quantity < qty) {
        return false; // Not enough of the ingredient
    }

    return true;
}

/**
* if there are enough ingredients in warehouse, then true
* @return  true if possible
*/
bool can_make(ordine *order) {
    ricetta *r = order->ricetta;
    unsigned int total_ingredients = r->total_ingredients;
    unsigned int order_quantity = order->quantita;

    for (unsigned int i = 0; i < total_ingredients; i++) {
        char *ingredient = r->ingrediente[i];
        int required_amount = r->quantita[i] * order_quantity;

        unsigned int index = hash(ingredient, magazzino->capacity);

        lotto_node *node = magazzino->table[index];

        while (node != NULL && strcmp(node->ingredient, ingredient) < 0) {
            node = node->next;
        }

        if (node == NULL || strcmp(node->ingredient, ingredient) != 0) {
            order->missing_ing = ingredient;
            order->missing_q = required_amount;
            return false; // Ingredient not found
        }
        lotto *current = node->head;
        int total_quantity = 0;

        while (current != NULL) {
            total_quantity += current->quantita;

            if (total_quantity >= required_amount) {
                // There is enough of the ingredient
                break;
            }
            current = current->next;
        }
        if (total_quantity < required_amount) {
            order->missing_ing = ingredient;
            order->missing_q = required_amount;
            return false; // Not enough of the ingredient
        }
    }
    return true;
}

void cook(const ordine *order) {
    ricetta *r = order->ricetta;
    for (unsigned int i = 0; i < r->total_ingredients; i++) {
        //for each ingredient
        char *ingredient = r->ingrediente[i];
        unsigned int required_amount = r->quantita[i] * order->quantita;

        // remove the required amount of the ingredient
        if (magazzino->table != NULL) {
            consume_ingredient(ingredient, required_amount);
        }
    }
}

void insert_order(ordini_arr *array, ordine *new_ord) {
    if (array->size == array->capacity) {
        array->capacity *= 2;
        array->orders = (ordine **) realloc(array->orders, array->capacity * sizeof(ordine *));
    }

    int i = array->size - 1;
    while (i >= 0 && array->orders[i]->time_added > new_ord->time_added) {
        array->orders[i + 1] = array->orders[i]; // shift elements to the right
        i--;
    }

    array->orders[i + 1] = new_ord;
    array->size++;
}

void order() {
    char nome[21];
    int quantita = 0;

    if (scanf("%s", nome) != 1) {
        printf("error reading name in order\n");
        return;
    }

    if (scanf("%d", &quantita) != 1) {
        printf("error. error reading quantity in order\n");
        return;
    }

    char c = getchar(); //I need to get rid of the newline character
    if (c == '\n') {
        c = getchar();
        if (c != EOF && c != '\n') {
            ungetc(c, stdin);
        }
    } else if (c != EOF) {
        ungetc(c, stdin);
    }

    ricetta *if_exists = search_receipt(nome);
    //search if a receipt with that name is present
    if (if_exists == NULL) {
        //not present
        printf("rifiutato\n");
        return;
    }

    ordine *new_ordine = create_ordine(quantita, if_exists); //compose the new receipt using input


    if (can_make(new_ordine)) {
        //if it is possible to prepare it now
        cook(new_ordine);
        insert_order(orders_pronti, new_ordine);
    } else {
        //not enough ingredients
        insert_order(orders_attesa, new_ordine);
    }
    printf("accettato\n");
}

/**
*read input and call add ingredients to the magazzino
*/
void rifornimento() {
    char ingredient[21];
    unsigned int quantity;
    unsigned int scadenza;

    while (scanf("%s %u %u", ingredient, &quantity, &scadenza) == 3) {
        if (scadenza >= t) {
            //evitare inserire quelli gia' scaduti
            insert_lotto(ingredient, scadenza, quantity);
        }
        char c = getchar();
        if (c == '\n' || c == EOF) {
            break;
        }
        ungetc(c, stdin); // put the character back if it's not a newline
    }
    //for all the orders in waiting list check if the order can be prepared
    char c = getchar();
    if (c != '\n' || c != EOF) {
        ungetc(c, stdin); // put the character back if it's not a newline
    }

    printf("rifornito\n");

    for (unsigned int i = 0; i < orders_attesa->size; i++) {
        ordine *curr_ord = orders_attesa->orders[i];
        if (one_available(curr_ord->missing_ing, curr_ord->missing_q)) {
            if (can_make(curr_ord)) {
                cook(curr_ord);
                move_order(curr_ord); // move the order from attesa to pronto
                i--; // adjust the index after deletion since the array size is reduced
            }
        }
    }
}

/**
* from the list ordine_pronti picks up the orders, that fit in the camion.
* analysis starts from the oldest ordest a.k.a. the smallest time_added
* @return pointer to an array
*/
void choose_orders(ordine ***selected_orders, int *num_orders) {
    unsigned int max_size = 10; // initial size
    *num_orders = 0;

    *selected_orders = (ordine **) malloc(max_size * sizeof(ordine *));

    unsigned int weight = 0;
    unsigned int count = 0;

    if (orders_pronti->size == 0) {
        // no orders in the list
        return;
    }

    for (unsigned int i = 0; i < orders_pronti->size && weight + orders_pronti->orders[i]->peso <= peso_max; i++) {
        if (count >= max_size) {
            max_size *= 2;
            ordine **temp = (ordine **) realloc(*selected_orders, max_size * sizeof(ordine *));
            *selected_orders = temp;
        }
        (*selected_orders)[count] = orders_pronti->orders[i];
        weight += orders_pronti->orders[i]->peso;
        count++;
    }

    *num_orders = count; //to preserve the number of elements
    if (count < max_size) {
        ordine **temp = (ordine **) realloc(*selected_orders, count * sizeof(ordine *));
        *selected_orders = temp;
    }

}

int compare_ord(const void *a, const void *b) {
    ordine *ord_a = *(ordine **) a;
    ordine *ord_b = *(ordine **) b;
    //decreasing weight
    if (ord_b->peso > ord_a->peso) {
        return 1;
    }

    if (ord_b->peso < ord_a->peso) {
        return -1;
    }
    //time_added
    if (ord_a->time_added < ord_b->time_added) {
        return -1;
    }
    if (ord_a->time_added > ord_b->time_added) {
        return 1;
    }
    return 0;
}

void riempire_camion() {
    int num = 0; //the ammount of the orders in the camion
    ordine **orders = NULL; //array of orders to analise

    choose_orders(&orders, &num); //to choose and copy

    if (num == 0) {
        printf("camioncino vuoto\n");
        return;
    }

    qsort(orders, num, sizeof(ordine *), compare_ord); //native function

    for (int i = 0; i < num; i++) {
        printf("%d %s %d\n", orders[i]->time_added, orders[i]->ricetta->nome, orders[i]->quantita);
    }

    // delete orders from the list
    for (int i = 0; i < num; i++) {
        delete_order(orders_pronti, orders[i]->time_added);
    }

    free(orders);
}

void distinguish_request(const char *request) {
    if (strcmp(request, "rifornimento") == 0) {
        rifornimento();
    } else if (strcmp(request, "ordine") == 0) {
        order();
    } else if (strcmp(request, "aggiungi_ricetta") == 0) {
        aggiunere_ricetta();
    } else if (strcmp(request, "rimuovi_ricetta") == 0) {
        rimuovere_ricetta();
    } else {
        printf("test. cannot distinguish the request. t = %d \n", t);
    }
}

int main() {
    ricette = create_rec_hash_table(0, 2048);
    orders_pronti = init_orders(1000);
    orders_attesa = init_orders(1000);
    magazzino = create_hash_table(0, 4096);

    if (scanf("%u", &periodo) != 1) {
        printf("test. error reading periodo\n");
        return 1;
    }
    if (scanf("%u", &peso_max) != 1) {
        printf("test. error reading peso_max\n");
        return 1;
    }

    //first elaboration is outside the while loop because I want to keep t=0 in the beginning
    if (scanf("%s", input_word) != 1) {
        printf("test. error reading input_word\n");
        return 1;
    }

    distinguish_request(input_word);
    t++;

    while (1) {
        char c = getchar();

        if (c == EOF || c == '\n') {
            if (t % periodo == 0) {
                riempire_camion();
            }
            break;
        }
        ungetc(c, stdin);

        if (scanf("%s", input_word) != 1) {
            printf("error. reading");
            return 1;
        }

        if (t % periodo == 0) {
            // arriva il corriere
            riempire_camion();
        }

        if (magazzino->table != NULL) {
            buttare_scaduto();
        }

        distinguish_request(input_word);
        t++;
    }
    free_receipt_list();

    free(orders_attesa->orders);
    free(orders_attesa);
    free(orders_pronti->orders);
    free(orders_pronti);
    free_hash_magazzino();

    return 0;
}
