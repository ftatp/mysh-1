#ifndef MYSH_COMMANDS_H_
#define MYSH_COMMANDS_H_

struct single_command
{
  int argc;
  char** argv;
};

int evaluate_command(int n_commands, struct single_command (*commands)[512]);

void free_commands(int n_commands, struct single_command (*commands)[512]);

int execute(struct single_command*);

char* get_input_from_parent(int*);
void put_output_to_child(int*, char*);

#endif // MYSH_COMMANDS_H_
