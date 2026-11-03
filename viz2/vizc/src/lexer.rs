pub mod error;
pub mod token;

use error::LexError;
use token::PrimType;
use token::Span;
use token::TokenKind;
use token::Token;

pub struct Lexer<'src> {
    src: &'src [u8],
    pos: usize,
}

#[allow(dead_code)]
impl<'src> Lexer<'src> {
    pub fn new(src: &'src str) -> Self {
        assert!(src.len() <= u32::MAX as usize);
        Self {
            src: src.as_bytes(),
            pos: 0,
        }
    }

    fn peek(&self) -> Option<u8> {
        self.src.get(self.pos).copied()
    }

    fn peek_next(&self) -> Option<u8> {
        self.src.get(self.pos + 1).copied()
    }

    fn advance(&mut self) -> Option<u8> {
        let byte = self.peek()?;
        self.pos += 1;
        Some(byte)
    }

    fn eat(&mut self, expected: u8) -> bool {
        if self.peek() == Some(expected) {
            self.pos += 1;
            true
        } else {
            false
        }
    }

    fn pos(&self) -> u32 {
        self.pos as u32
    }

    fn is_at_end(&self) -> bool {
        self.pos >= self.src.len()
    }

    fn span_from(&self, start: u32) -> token::Span {
        token::Span {
            start,
            end: self.pos(),
        }
    }

    fn skip_trivia(&mut self) -> Result<(), LexError> {
        loop {
            match self.peek() {
                Some(b' ' | b'\t' | b'\n' | b'\r') => {
                    self.advance();
                }
                Some(b'/') if self.peek_next() == Some(b'/') => loop {
                    match self.peek() {
                        None | Some(b'\n') => break,
                        _ => {
                            self.advance();
                        }
                    }
                },
                Some(b'/') if self.peek_next() == Some(b'*') => {
                    let start = self.pos();
                    self.advance();
                    self.advance();
                    loop {
                        match self.peek() {
                            None => {
                                return Err(LexError::UnterminatedComment(Span {
                                    start,
                                    end: start + 2,
                                }));
                            }
                            Some(b'*') if self.peek_next() == Some(b'/') => {
                                self.advance();
                                self.advance();
                                break;
                            }
                            _ => {
                                self.advance();
                            }
                        }
                    }
                }
                _ => return Ok(()),
            }
        }
    }

    fn lex_ident(&mut self) -> Result<TokenKind, LexError> {
        let start = self.pos;
        while self.peek().is_some_and(is_ident_byte) {
            self.advance();
        }
        let span = self.span_from(start as u32);
        let result = &self.src[start..self.pos];
        if result[0].is_ascii_lowercase() {
            return Ok(match result {
                b"dec" => TokenKind::Dec,
                b"struct" => TokenKind::Struct,
                b"fn" => TokenKind::Fn,
                b"var" => TokenKind::Var,
                b"exec" => TokenKind::Exec,
                b"load" => TokenKind::Load,
                b"mut" => TokenKind::Mut,
                b"pk" => TokenKind::Pk,
                b"return" => TokenKind::Return,
                b"if" => TokenKind::If,
                b"else" => TokenKind::Else,
                b"while" => TokenKind::While,
                b"for" => TokenKind::For,
                b"continue" => TokenKind::Continue,
                b"break" => TokenKind::Break,
                b"unterm" => TokenKind::Unterm,
                b"undef" => TokenKind::Undef,
                b"i8" => TokenKind::Prim(PrimType::I8),
                b"i16" => TokenKind::Prim(PrimType::I16),
                b"i32" => TokenKind::Prim(PrimType::I32),
                b"i64" => TokenKind::Prim(PrimType::I64),
                b"u8" => TokenKind::Prim(PrimType::U8),
                b"u16" => TokenKind::Prim(PrimType::U16),
                b"u32" => TokenKind::Prim(PrimType::U32),
                b"u64" => TokenKind::Prim(PrimType::U64),
                b"usize" => TokenKind::Prim(PrimType::Usize),
                b"isize" => TokenKind::Prim(PrimType::Isize),
                _ => TokenKind::LowIdent,
            });
        } else {
            if !result.iter().any(|b| b.is_ascii_lowercase()) {
                return Ok(TokenKind::AllcapsIdent);
            }
            if result[1].is_ascii_lowercase() {
                Ok(TokenKind::CapIdent)
            } else {
                Err(LexError::BadIdentCase(span))
            }
        }
    }

    fn lex_number(&mut self) -> Result<TokenKind, LexError> {
        let start = self.pos;
        let radix: u32 = if self.peek() == Some(b'0') && self.peek_next() == Some(b'x') {
            self.advance(); self.advance();
            16
        } else if self.peek() == Some(b'0') && self.peek_next() == Some(b'b') {
            self.advance(); self.advance();
            2
        } else {
            10
        };
        let digits_start = self.pos;

        while self.peek().is_some_and(is_ident_byte) {
            self.advance();
        }
        
        let digits = &self.src[digits_start..self.pos];
        let span = self.span_from(start as u32);

        if digits.len() == 0 { return Err(LexError::BadIntLiteral(span)); }
        if radix == 10 && digits.len() > 1 && digits[0] == b'0' { return Err(LexError::BadIntLiteral(span)); }

        let mut value: u64 = 0;
        for &b in digits {
            let d = (b as char).to_digit(radix).ok_or(LexError::BadIntLiteral(span))?;
            value = value.checked_mul(radix as u64)
                .and_then(|v| v.checked_add(d as u64))
                .ok_or(LexError::IntOverflow(span))?;
        }
        Ok(TokenKind::IntLit(value))
    }

    fn lex_string(&mut self) -> Result<TokenKind, LexError> {
        let start = self.pos;
        let span = Span { start: start as u32, end: start as u32 + 1 };
        self.advance();
        loop {
            match self.peek() {
                Some(b'"') => {
                    self.advance();
                    return Ok(TokenKind::StrLit);
                },
                Some(b'\\') => {
                    let esc = Span { start: self.pos(), end: self.pos() + 1 };
                    return Err(LexError::UnsupportedEscape(esc));
                }
                None | Some(b'\n') => {
                    return Err(LexError::UnterminatedString(span));
                },
                _ => { self.advance(); },
            }
        }
    }

    fn lex_punct(&mut self) -> Result<TokenKind, LexError> {
        let start = self.pos();
        Ok(match self.advance() {
            Some(b'(') => TokenKind::Lparen,
            Some(b')') => TokenKind::Rparen,
            Some(b'[') => TokenKind::Lbrack,
            Some(b']') => TokenKind::Rbrack,
            Some(b'{') => TokenKind::Lbrace,
            Some(b'}') => TokenKind::Rbrace,
            Some(b',') => TokenKind::Comma,
            Some(b';') => TokenKind::Semicolon,
            Some(b'.') => TokenKind::Dot,
            Some(b':') => TokenKind::Colon,
            Some(b'$') => TokenKind::Dollar,
            Some(b'+') => if self.eat(b'=') { TokenKind::PlusEq } else { TokenKind::Plus },
            Some(b'-') => {
                if self.eat(b'=') { TokenKind::MinusEq }
                else if self.eat(b'>') { TokenKind::Arrow }
                else { TokenKind::Minus }
            }
            Some(b'*') => if self.eat(b'=') { TokenKind::StarEq } else { TokenKind::Star },
            Some(b'/') => if self.eat(b'=') { TokenKind::FslashEq } else { TokenKind::Fslash },
            Some(b'%') => if self.eat(b'=') { TokenKind::PcentEq } else { TokenKind::Pcent },
            Some(b'=') => if self.eat(b'=') { TokenKind::DoubleEquals } else { TokenKind::Equals },
            Some(b'!') => if self.eat(b'=') { TokenKind::Notequal } else { TokenKind::Exclaim },
            Some(b'<') => if self.eat(b'=') { TokenKind::LthanEqu } else { TokenKind::Lthan },
            Some(b'>') => if self.eat(b'=') { TokenKind::GthanEqu } else { TokenKind::Gthan },
            Some(b'&') => if self.eat(b'&') { TokenKind::DoubleAmper } else { TokenKind::Amper },
            Some(b'|') => if self.eat(b'|') { TokenKind::DoublePipe } else {
                return Err(LexError::UndefinedToken(Span{ start, end: start + 1 }))
            }
            _ => { return Err(LexError::UnexpectedByte(Span { start, end: start + 1 })); }
        })
    }

    fn lex_builtin(&mut self) -> Result<TokenKind, LexError> {
        let start = self.pos(); self.advance();
        if !self.peek().is_some_and(|b| b.is_ascii_alphabetic()) { return Err(LexError::BadBuiltinCall(Span { start, end: start + 1 })); }
        while self.peek().is_some_and(is_ident_byte) { self.advance(); }
        Ok(TokenKind::Builtin)
    }

    pub fn next_token(&mut self) -> Result<Token, LexError> {
        self.skip_trivia()?;
        let start = self.pos();
        if self.is_at_end() {
            return Ok(Token { token_kind: TokenKind::Eof,
                         span: Span {
                             start: start, end: start
                         }});
        }
        let kind = match self.peek() {
            Some(b) if b.is_ascii_alphabetic() => self.lex_ident()?,
            Some(b) if b.is_ascii_digit() => self.lex_number()?,
            Some(b'"') => self.lex_string()?,
            Some(b'@') => self.lex_builtin()?,
            Some(_) => self.lex_punct()?,
            None => TokenKind::Eof,
        };
        Ok(Token { token_kind: kind, span: self.span_from(start) })
    }
}

fn is_ident_byte(b: u8) -> bool {
    b.is_ascii_alphanumeric() || b == b'_'
}

pub fn tokenize(src: &str) -> Result<Vec<Token>, LexError> {
    let mut lex = Lexer::new(src);
    let mut toks: Vec<Token> = Vec::new();
    loop {
        let tok = Lexer::next_token(&mut lex)?;
        toks.push(tok);
        if tok.token_kind == TokenKind::Eof { break; }
    }
    Ok(toks)
}
