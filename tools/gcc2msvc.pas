const MAXLINES = 30000;
      RESERVED_REGS = 4;
      MARKER   = '###END';
{ 
  a file with a list of null-terminated strings, representing the x86 
  instruction set 
  }
      ilistfile= 'x86instructionlist'; 
      max_instructions = 600; { actually, 588}
      
      END_OF_ASM_BLOCK = '}';
      whitespace = [#9, #13, ' '];

type FImage   = Array [1..MAXLINES] of String;
     AsmBlock = Array [0..9] of String;
     IList    = Array [1..max_instructions] of string;
     RegLists = array [1..RESERVED_REGS] of string;
     
const Reserve : RegLists = ('ebx', 'ebp', 'esi', 'edi');
     
var buff1, buff2 : FImage;
    insns : IList;


Procedure ReadPhile(fn : string; var a : FImage);
    var t : text;
	l : longint;
    begin
    assign(t, fn);
    reset(t);
    l := 0;
    while not eof(t) do
	begin
	inc(l);
	readln(t, a[l]);
	end;
    inc(l);
    a[l] := MARKER;
    close(t);
    end;

Procedure WritePhile(fn : string; var a : FImage);
    var t : text;
	i : longint;
    begin
    assign(t, fn);
    rewrite(t);
    i := 1;
    while a[i]<>MARKER do
	begin
	writeln(t, a[i]);
	inc(i);
	end;
    close(t);
    end;
    
Procedure Init;
    var f : file of byte;
	buff : array [0..MAX_INSTRUCTIONS*10] of char;
	p, i, j, fs : longint;
    begin
    assign(f, ilistfile);
    reset(f);
    fs := filesize(f);
    blockread(f, buff, fs);
    close(f);
    p := -1;
    j := 0;
    repeat
	inc(p);
	if p >= fs then break;
	i := p;
	while buff[i]<>#0 do inc(i);
	inc(j);
	insns[j] := '';
	while p < i do 
	    begin
	    insns[j] := insns[j] + buff[p];
	    inc(p);
	    end;	
	until false;
    end;

Procedure Convert_assembly_format(var a, b : Fimage; l1, l2 : longint; var l : Longint; fill : string);
    var outlist, inlist, modlist : string;
	i, j, n, endej, endei : longint;
	savereg : array [1..RESERVED_REGS] of boolean;
	replc: asmblock;
	
    Procedure swaps(var a , b : string);
     var c : string;
     begin
     c := a;
     a := b;
     b := c;
     end;
	
    Function GetSection : string;
	var r : string;
	begin
	r := '';
	dec(i);
        repeat
    	    while (i > 0) and (a[j][i]<>':') do 
		begin
		r := a[j][i] + r;
		dec(i);
		end;
	    if i = 0 then
		begin
		dec(j);
		i := length(a[j]);
		end else break;
	    until false;
	GetSection := r;
	end;
	
    Function ReFormat( s : String) : string;
	var i, n, k : longint;
	    ops  : array [1..4] of string;
	    mne  : string;
	    r    : string;
	    
	Function Scans(var si, so : string) : boolean;
	    begin
	    scans := FALSE;
	    while (length(si)>0 ) and (s[1] in whitespace) do delete(si, 1,1);
	    if length(si) = 0 then exit;
	    scans := TRUE;
	    so := '';
	    while (length(si)>0) and (not(s[1] in whitespace)) do 
		begin
		so := so + si[1];
		delete(si, 1, 1);
		end;
	    if so[length(so)]=',' then delete(so, length(so), 1);
	    end;
	    
	Function Fix_memop_arg(s : string) : String;
	    begin
	    if s[1] = '%' then
		begin
		if s[2] <> '%' then
		    begin
		    WriteLn('Assembly block from: ',l1,': cannot use %(digit) in memops!!!');
		    halt(1);
		    end
		    else
		    begin
		    Fix_Memop_arg := copy(s, 3, 255);
		    end;
		end
		else
		begin
		if s[1] <> '$' then
		    begin
		    WriteLn('Assembly block from: ',l1, ': illegal char for arg in memop');
		    halt(1);
		    end;
		fix_memop_arg := copy(s, 2, 255);
		end;
	    end;
	    
	Function Handle_memop(s : string) : string;
	    var 
		base : string;
		offset:string;
		size : string;
		displacement : string;
		r : string;
		tt : longint;
	    begin
	    if pos('(', s) = 0 then
		begin
		writeln('Assembly block from: ',l1, ': expected memop');
		halt(1);
		end;
	    {
		memop in `s' is of the kind:
		
		0x48(%%ebx, %%esi, $4)
		or
		(%%esi, %%eax, $2)
		or
		(%%eax)
		etc., which should map to
		
		[ebx + esi*4 + 0x48]
		[esi + eax*2]
		[eax]
		etc..
	    }
	    displacement := '';
	    base := '';
	    offset := '';
	    size := '';
	    if s[1] <> '(' then
		begin
		displacement := copy(s, 1, pos('(', s)-1);
		delete(s, 1, pos('(', s)-1);
		end;
	    delete(s,1,1);
	    tt := pos(')', s);
	    if tt = 0 then
		begin
		writeln('Assembly block from: ', l1,': missing ")"!');
		halt(1);
		end;
	    s[tt] := ',';
	    if length(s) > tt then writeln('Warning, trailing chars after memop');
	    s := copy(s, 1, tt);
	    base := copy(s, 1, pos(',',s)-1);
	    delete(s,1,pos(',',s));
	    tt := pos(',', s);
	    if tt <> 0 then
		begin
		offset := copy(s, 1, tt-1);
		delete(s,1,tt);
		tt := pos(',',s);
		if tt <> 0 then
		    begin
		    size := copy(s, 1, tt-1);
		    delete(s,1,tt);
		    if s <> '' then
			begin
			WriteLn('Assembly block from: ',l1,' excessive operands in memop');
			halt(1);
			end;
		    end;
		end;
	    r := '[' + fix_memop_arg(base);
	    if offset <> '' then r := r + ' + '+fix_memop_arg(offset);
	    if size <> '' then r := r + '*'+fix_memop_arg(size);
	    if displacement <> '' then r := r + ' + ' + displacement;
	    Handle_memop := r+']';
	    end;	    
	    

	Function Handle_offset(s : String) : String;
	    var k : longint;
		base : string;
	    begin
	    {
		offset is of the form:
		4%2
		0x8%0
		...etc
		should be mapped to
		[m + 4]
		[src+0x8] etc... 
	    }
	    if pos('%', s)= 0 then
		begin
		Writeln('Assembly block from: ',l1,' simple offset is of unexpected form!');
		halt(1);
		end;
	    k := ord(s[pos('%',s)+1]) - ord('0');
	    if (k < 0) or (k > 9) then
		begin
		WriteLn('Assembly block from: ',l1,' simple offset is of unexpected form! (non-int)');
		halt(1);
		end;
	    base := replc[k];
	    if base[1] = '*' then delete (base, 1,1);
	    delete(s, pos('%', s), 255);
	    Handle_Offset := '['+base+' + '+s+']';
	    end;
	    
	begin
	r := #9;
	while pos('\n', s) <> 0 do delete(s, pos('\n', s), 2);
	while pos('\t', s) <> 0 do delete(s, pos('\t', s), 2);
	if pos('(', s) <> 0 then
	    begin
	    i := pos('(', s);
	    while s[i] <> ')' do 
		begin
		if s[i] in whitespace then delete(s, i, 1)
				      else inc(i);
		end;
	    end;
	if not scans(s, mne) then
	    begin writeln('Assembly block from ', l1,': no mnemonic found!'); halt(1); end;
	n := 1;
	while scans(s, ops[n]) do
	    begin
	    inc(n);
	    if n = 5 then
		begin
		writeln('Excessive arguments: ',mne,' ',ops[1],' ',ops[2],' ', ops[3], ' ', ops[4],' ...');
		halt(1);
		end;
	    end;
	dec(n);
	if n = 2 then swaps(ops[1], ops[2]) else
	    if n = 3 then swaps(ops[1], ops[3]);
	if mne[1] <> '.' then
	    begin
	     {TODO: check for /l, /w, /s, etc. suffix and remove if existent }
	    end;
	r := r + mne;
	for i := 1 to n do
	    begin
	    r := r + #9 + #9;
	    if ops[i][1] = '%' then
		begin
		if ops[i][2] = '%' then r := r + copy(ops[i], 3, 255)
		 else
		 begin
		 k := ord(ops[i][2]) - ord('0');
		 if (k<0) or (k>9) then
		    begin
		    writeln('Assembly block from: ',l1,': nonmatched argument: ',ops[i]);
		    halt(1);
		    end;
		 if replc[k][1] = '*' then r := r + '[' + copy(replc[k], 2, 255) + ']'
				      else r := r + replc[k];
		 end;
		end
		else
		begin
		case ops[i][1] of
		    '(': r := r + handle_memop(ops[i]);
		    '$': if pos('(', ops[i])=0 then r := r + copy(ops[i], 2, 255)
			else r := r + handle_memop(copy(ops[i],2,255));
		    '0'..'9': if pos('(', ops[i])<>0 then r := r + handle_memop(ops[i])
						     else r := r + handle_offset(ops[i]);
		    else r := r + ops[i];
		    end;
		end;
	    if i < n then r := r+',';
	    end;
	ReFormat := r;
	end;
	
    Function Parse_single_line(s : string ) : String;
	var 
	    i, j : longint;
	begin
	if pos('__asm __volatile', s)<>0 then
	    begin
	    delete(s, pos('__asm __volatile', s),16);
	    while (length(s)>0) and (s[1] <> '(') do delete(s,1,1);
	    if length(s) = 0 then	
		begin
		WriteLn('String Exhausted unexpectedly');
		halt(1);
		end else delete(s,1,1);
	    end;
	
	if (pos('"', s)=0) then
	    begin
	    Parse_single_line := s;
	    exit;
	    end;
	i := pos('"', s);
	s[i] := '^';
	j := pos('"', s);
	s[i] := '"';
	if (j=0) then
	    begin
	    writeLn('Assembly block from ', l1, '... line with no matching quote');
	    halt(1);
	    end;
	Parse_single_line := copy(s, 1, i-1) + ReFormat(copy(s, i+1, j-i-1)) + copy(s, j+1, 255);
	end;
	
    begin
    j := l2;
    i := pos(');', a[l2]);
    { get asm(:::) block sections }
    ModList := GetSection;
    InList  := GetSection;
    OutList := GetSection;
    endej := j;
    endei := i-1;
    
    { check if we should save some registers... }    
    for i := 1 to RESERVED_REGS do 
	savereg[i] := Pos('"'+Reserve[i]+'"', ModList)<>0;
    for i := 1 to RESERVED_REGS do {push anything if necessary}
	if savereg [i] then
	    begin
	    b[l] := fill + #9 + 'push' + #9+#9 + Reserve[i];
	    inc(l);
	    end;
	    
    n := 0;
    while pos('(', OutList) <> 0 do
	begin
	replc[n] := Copy(OutList, pos('(', OutList) + 1, pos(')', OutList) - pos('(', OutList) - 1);
	delete(outList, 1, pos(')', OutList));
	inc(n);
	end;
    while pos('(', InList) <> 0 do
	begin
	replc[n] := Copy(InList, pos('(', InList) + 1, pos(')', InList) - pos('(', InList) - 1);
	delete(InList, 1, pos(')', InList));
	inc(n);
	end;
    { process the assembly code }

    for j := l1 to endej - 1 do
	begin
	b[l] := parse_single_line(a[j]);
	inc(l);
	end;

    { process last line }    
    j := endej;
    b[l] := parse_single_line(copy(a[j], 1, endei));
    inc(l);


    for i := RESERVED_REGS downto 1 do {pop out any registers if necessary}
	if savereg [i] then
	    begin
	    b[l] := fill + #9 + 'pop' + #9+#9 + Reserve[i];
	    inc(l);
	    end;
    end;    
    
Procedure Convert(var a,b : FImage);
    var i, j, p, q : longint;
	ablocks, alines : longint;
	st, stt		: string;
	inComment : boolean;
	

    Procedure ReParse(var so : String; si : String);
	var plc, pbc : integer;
	    j : integer;
	begin
	plc := pos('//', si);
	pbc := pos('/*', si);
	if plc+pbc = 0 then 
	    begin
	    so := so + si;
	    exit;
	    end;
	if (plc<>0) and (pbc = 0) then
	    begin
	    so := so + si;
	    exit;
	    end;
	if (plc<>0) and (plc < pbc) then
	    so := so + si
	    else
	    begin
	    j := pbc;
	      repeat
	  	so := so + copy(si, 1, j+1);
		inComment := TRUE;
		delete(si, 1, j+1);
		j := pos('*/', si);
		if j = 0 then break;
		so := so + copy(si, 1, j+1);
		inComment := FALSE;
		delete(si, 1, j+1);
		j := pos('/*', si);
		until j = 0;
	    so := so + si;
	    end;
	end;	
	
    begin
    b := a;
    ablocks := 0; alines := 0; inComment := false;
    i := 1; j := 1;
    while a[i] <> MARKER do
	begin
{	Write(' #',i:4,': ',byte(inComment),';');}
	
	if inComment then
	    begin
	    if pos('*/', a[i])=0 then
		begin
		b[j] := a[i];
		inc(j);
		end else
		begin
		b[j] := copy(a[i], 1, pos('*/', a[i])+1);
		inComment := FALSE;
		ReParse(b[j], copy(a[i], pos('*/', a[i])+2, 255));
		inc(j);	
		end;
	    end else
	    begin
	    if pos('__asm __volatile', a[i])<>0 then
		begin
		if pos('emms', a[i])<>0 then
		    begin
		    { handle single EMMS instruction }
		    st := copy(a[i], 1, pos('__asm', a[i])-1);
		    b[j  ] := st + 'asm {';
		    b[j+1] := st + #9 + 'emms';
		    b[j+2] := st + END_OF_ASM_BLOCK;
		    inc(j, 3);
		    inc(alines);
		    inc(ablocks);
		    end
		    else
		    begin
		    { real assembly }
		    inc(ablocks);
		    p := i;
		    q := p;
		    while (a[q]<>MARKER) and (pos(');', a[q]) = 0) do inc(q);
		    if a[q] = MARKER then
			begin
			writeLn('Cannot find end of asm block starting at ', p,'!');
			halt(1);
			end;
		    inc(alines, q - p + 1);
		    st := copy(a[i], 1, pos('__asm', a[i])-1);
		    b[j] := st + 'asm {';
		    inc(j);
		    convert_assembly_format(a, b, p, q, j, st);
		    b[j] := st + END_OF_ASM_BLOCK;
		    inc(j);	    
		    i := q;
		    stt := copy(a[i], pos(');', a[i])+2, 255);
		    if stt <> '' then
			begin
			b[j]:='';
			ReParse(b[j], stt);
			inc(j);
			end;
		    end;
		end
		else
		begin
		b[j] := '';
		ReParse(b[j], a[i]);
		inc(j);
		end;
	    end;
	inc(i);
	end;
    b[j] := MARKER;
    WriteLn('Parsing done, ',ablocks, ' assembly blocks, ', alines, ' assembly lines.');
    end;

BEGIN
WriteLn('gcc2msvc - convert c/cpp files with AT&T assembly to intel-style');
If ParamCount = 0 then
    begin

	WriteLn;
	WriteLn('Usage:');
	WriteLn('        gcc2msvc <input_file> <output_file>');
	WriteLn(' or');
	WriteLn('        gcc2msvc <file> (backups the original file to <file.bak>)');
	WriteLn;
	WriteLn('Error: not enough parameters');
	halt(1);
    end;
Init;
ReadPhile(ParamStr(1), buff1);
if ParamCount = 1 then
    begin
    WritePhile(ParamStr(1)+'.bak', buff1);
    Convert(buff1, buff2);
    WritePhile(ParamStr(1), buff2);
    end
    else
    begin
    Convert(buff1, buff2);
    WritePhile(ParamStr(2), buff2);
    end;
END.
