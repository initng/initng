int is_same_env_var(char *var1, char *var2)
{
	int i = 0;

	if (!var1 || !var2)
		return 0;	/* bad error checking in caller! */

	for (i = 0; var1[i] && var2[i] && var1[i] != '=' && var1[i] == var2[i];
	     i++);

	return var1[i] == var2[i];
}
