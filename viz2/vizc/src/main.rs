pub mod lexer;
fn main() {
    let Some(path) = std::env::args().nth(1) else {
        eprintln!("usage: vizc <file.vz>");
        std::process::exit(1);
    };
    let src = match std::fs::read_to_string(&path) {
        Ok(s) => s,
        Err(e) => {
            eprintln!("vizc: cannot read {}: {}", path, e);
            std::process::exit(1);
        }
    };
    match lexer::tokenize(src.as_str()) {
        Ok(n) => {
            for t in &n {
                let text = &src[t.span.start as usize..t.span.end as usize];
                println!("{:?} {:?}", t.token_kind, text);
            }
        }
        Err(e) => {
            eprintln!("{}", e.render(&path, &src));
            std::process::exit(1);
        }
    }
    
}
