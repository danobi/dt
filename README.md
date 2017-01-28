# tr
A quicker way to change directories

# note
This project is currently incomplete. `tr` can't actually change the shell
directory yet, but plan of action has been hashed out.

1. Have `tr` create a temporary file with `cd <dest_dir>`.
2. Wrap `tr` inside a shell script.
3. Following execution of the `tr` binary, have the shell script source the temporary file.
