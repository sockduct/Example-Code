#! /bin/bash
#
# James Small
# SDEV-415-81, Fall Semester 2020
# Week 14/15 Assignment
# Date:  December 18, 2020
#
# Lookup(3) - display datafile in sorted order
# - Sort "datafile" by last names
# - Show the user the contents of "datafile"
# - Tell the user the number of entries in the file
# - Ask the user if he or she would like to add an entry to datafile.  If the
#   answer is yes or y:
#   . Prompt the user for a new name, phone number, address, birth date, and
#     salary.  Each item will be stored in a separate variable. Script
#     provides the colons between the fields and appends the information to
#     the "datafile".
#   . Resort the file by last names.
#   . Tell the user script added the entry, and show him or her the line
#     preceded by the line number.
# - Check to see if the "datafile" exists and if it is readable and writable
# - Add a menu to the "lookup" script to resemble the following:
#   . [1] Add entry
#   . [2] Delete entry
#   . [3] View entry
#   . [4] Exit
#
 
# Environment/Variables
export LC_ALL=en_US.UTF-8  # For US numbers, use comma for thousands separator
datafile="datafile"
declare -a menu_items=(
   "Add entry"
   "Delete entry"
   "View entry"
   "Show all entries"
   "Quit"
)
# Menu Input Prompt
PS3="Please make a selection: "

# File exists, is readable, and isn't empty
valid_file() {
   if [[ ! -f "$1" || ! -r "$1" ]]; then
      printf "\nError:  Expected %s to be a readable, regular file.\n" "$1"
      exit 1
   fi

   if [[ $# -eq 2 && "$2" = "writable" && ! -w "$1" ]]; then
      printf "\nError:  Expected %s to be a writable file.\n" "$1"
      exit 1
   fi

   if [[ ! -s "$1" ]]; then
      printf "\n%s empty.\n" "$1"
      return 1
   fi
}

# Create a sorted version of $1, sort by last name:
sort_records() {
   if ! valid_file "$1" "writable"; then
      return 1
   fi

   local tempfile="$1".$$
   if ! touch "$tempfile" && [ ! -w "$tempfile" ]; then
      printf "\nError:  Expected to be able to write to %s.\n" "$tempfile"
      exit 1
   fi

   awk -F'[: ]' '{print $2","$0}' "$1" | sort | cut -d, -f2- > "$tempfile"
   mv -f "$tempfile" "$1"
   printf "\nSorted entries in %s.\n" "$1"
}

record_count() {
   if ! valid_file "$1"; then
      return 1
   fi

   lines=$(wc -l "$1" | cut -d' ' -f1)
   printf "\n%d entries in %s.\n\n" "$lines" "$1"
}

# Display contents of $1:
display_records() {
   if ! valid_file "$1"; then
      return 1
   fi

   awk -F: 'BEGIN {
               field1title=" -= Name =- ";
               field2title="Phone Number";
               field3title=" -= Address =- ";
               field4title="Birth Date";
               field5title="Salary";
               field1len=length(field1title);
               field2len=length(field2title);
               field3len=length(field3title);
               field4len=length(field4title);
               field5len=length(field5title);
               #
               # header is used to create a header line between the title row
               # and the entry rows; each element will end up with a number of
               # characters matching the field width:
               header[1]="=";
               header[2]="=";
               header[3]="=";
               header[4]="=";
               header[5]="="
            }

            {
               col1len=length($1);
               col2len=length($2);
               col3len=length($3);
               col4len=length($4);
               #
               # Account for the fact that we add thousands separators when we
               # print out this field:
               col5len=length(sprintf("%'"'"'d", $5));
               #
               # Calculate maximum field widths:
               field1len=field1len > col1len ? field1len:col1len;
               field2len=field2len > col2len ? field2len:col2len;
               field3len=field3len > col3len ? field3len:col3len;
               field4len=field4len > col4len ? field4len:col4len;
               field5len=field5len > col5len ? field5len:col5len;
               #
               # Store all file entries for print out later:
               record[NR]=$0
            }

            END {
               printf "\n%-*s | %-*s | %-*s | %-*s | %*s\n", field1len,
                      field1title, field2len, field2title, field3len,
                      field3title, field4len, field4title, field5len,
                      field5title;
               headerlen[1]=field1len;
               headerlen[2]=field2len;
               headerlen[3]=field3len;
               headerlen[4]=field4len;
               headerlen[5]=field5len;
               for (i = 1; i <= 5; i++) {
                  for (j = 1; j < headerlen[i]; j++) {
                     # Make header[n] match respective field n max width
                     header[i]=header[i] "="
                  }
               }
               printf "%s=|=%s=|=%s=|=%s=|=%s\n", header[1],
                      header[2], header[3], header[4], header[5];
               for (i = 1; i <= NR; i++) {
                  $0=record[i];
                  printf "%-*s | %-*s | %-*s | %*s | %'"'"'*d\n", field1len,
                         $1, field2len, $2, field3len, $3, field4len, $4,
                         field5len, $5
               }
            }' "$1"

   record_count "$1"
}

# Basic input validation
get_input() {
   while true; do
      read -p "$1" input
      if [[ ${#input} -ge $2 ]]; then
         break;
      fi
      printf "Error:  Input too short.  Expecting something like this:  %s\n\n" "$3"
   done
}

# Add new record to $1:
add_record() {
   # Not checking status - don't care if $1 empty:
   valid_file "$1" "writable"

   printf "\nNew entry:\n"
   # Prompt for fields:
   get_input "Name (First Last):  " 3 "George Smith"
   name=$input

   # Case-insensitive check for existing entry with same name:
   if grep -Eiq "^$input:" "$1"; then
      printf "Error:  '%s' already exists in %s:\n" "$name" "$1"
      find_record "$name" "$1" "quiet"
      return 1
   fi

   get_input "Phone Number (aaa-ppp-dddd):  " 12 "314-239-9137"
   phonenumber=$input
   get_input "Address (### Example Street, City, ST ZIPCode):  " 18 "123 Any Street, Georgetown, TX 78626"
   address=$input
   get_input "Birth Date (MM/DD/YY):  " 6 "3/21/68"
   birthdate=$input
   get_input "Salary (#####):  " 4 "43738"
   salary=$input

   # Append record:
   record="$name:$phonenumber:$address:$birthdate:$salary"
   printf "%s\n" "$record" >> "$1"

   printf "\nAdded entry to %s.\n" "$1"
}

# Find record (case-insensitive) $1 in $2:
# Optional 3rd argument "quiet" for no "*matching entry*" output
find_record() {
   if ! valid_file "$2"; then
      return 1
   fi

   if [[ $# -eq 3 && $3 = "quiet" ]]; then
      verbose=0
   else
      verbose=1
   fi

   # Case-insensitive check for single matching entry:
   count=$(grep -Eic "$1" "$2")
   if [[ $count -eq 1 && $verbose -eq 1 ]]; then
      printf "\nOne matching entry for '%s' in %s:\n" "$1" "$2"
   elif [[ "$count" -eq 0 && $verbose -eq 1 ]]; then
      printf "\nNo matching entries for '%s' in %s.\n" "$1" "$2"
      return 1
   else
      if [[ $verbose -eq 1 ]]; then
         printf "\nMultiple matching entries for '%s' in %s:\n" "$1" "$2"
      fi
   fi

   awk -F: -v targetrec="$1" 'tolower($0) ~ tolower(targetrec) {printf "%d:  %s, %s, %s, %s, %s\n",
                             NR, $1, $2, $3, $4, $5}' "$2"
}

delete_record() {
   if ! valid_file "$1" "writable"; then
      return 1
   fi

   printf "\nDelete entry within %s.\n" "$1"
   get_input "Please enter a name, phone number, address, or birth data to look for:  " 3 "Smith"

   # Case-insensitive check for single matching entry:
   count=$(grep -Eic "$input" "$1")
   if [[ $count -ne 1 ]]; then
      printf "Error:  Must be exactly one matching record in %s.\n" "$1"
      find_record "$input" "$1"
      return 1
   else
      entry=$(find_record "$input" "$1" "quiet")
   fi

   sed -i "/$input/Id" "$1"

   printf "\nDeleted entry '%s' in %s:\n%s\n" "$input" "$datafile" "$entry"
}

lookup_record() {
   if ! valid_file "$1"; then
      return 1
   fi

   printf "\nEntry lookup within %s.\n" "$1"
   get_input "Please enter a name, phone number, address, or birth data to look for:  " 2 "Smith"
   find_record "$input" "$1"
}

#
# Main Logic:
if sort_records "$datafile"; then
   display_records "$datafile"
fi
#
# Menu options:
while true; do
   printf "\n"
   select item in "${menu_items[@]}"; do
      case $REPLY in
         1) # Add entry
            if add_record "$datafile"; then
               newrec="$record"
               sort_records "$datafile"
               printf "\nNew entry:\n"
               find_record "$newrec" "$datafile" "quiet"
            fi
            break
            ;;
         2) # Delete entry
            delete_record "$datafile"
            break
            ;;
         3) # View entry
            lookup_record "$datafile"
            break
            ;;
         4) # Show all contacts
            display_records "$datafile"
            break
            ;;
         5|q|Q|e|E|x|X) # Quit
            printf "\nGoodbye.\n\n"
            exit 0
            ;;
         *) # Catch-all
            printf "\nInvalid entry, try again.\nPlease choose item 1 - 5.\n\n"
            break
            ;;
      esac
   done
done
