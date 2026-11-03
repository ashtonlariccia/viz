use super::token::Span;
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum LexError {
    UnterminatedComment(Span),
    BadIdentCase(Span),
    IntOverflow(Span),
    BadIntLiteral(Span),
    UnsupportedEscape(Span),
    UnterminatedString(Span),
    UndefinedToken(Span),
    UnexpectedByte(Span),
    BadBuiltinCall(Span),
}

impl LexError {
    pub fn span(&self) -> Span {
        match self {
            LexError::UnterminatedComment(s)
            | LexError::BadIdentCase(s)
            | LexError::IntOverflow(s)
            | LexError::BadIntLiteral(s)
            | LexError::UnsupportedEscape(s)
            | LexError::UnterminatedString(s)
            | LexError::UndefinedToken(s)
            | LexError::UnexpectedByte(s)
            | LexError::BadBuiltinCall(s)
            => *s,
        }
    }

    pub fn message(&self) -> &'static str {
        match self {
            LexError::UnterminatedComment(_) => "unterminated comment",
            LexError::BadIdentCase(_) => "bad identifier case",
            LexError::IntOverflow(_) => "integer overflow",
            LexError::BadIntLiteral(_) => "bad integer literal",
            LexError::UnsupportedEscape(_) => "escape sequences unsupported",
            LexError::UnterminatedString(_) => "unterminated string",
            LexError::UndefinedToken(_) => "undefined token",
            LexError::UnexpectedByte(_) => "unexpected byte",
            LexError::BadBuiltinCall(_) => "expected a builtin name"
        }
    }

    pub fn render(&self, file: &str, src: &str) -> String {
        let span = self.span();
        let (line, col) = line_col(src, span.start);
        let msg = self.message();

        let bytes = src.as_bytes();
        let start = (span.start as usize).min(bytes.len());

        let line_start = src[..start].rfind('\n').map_or(0, |i| i + 1);
        let line_end = src[start..].find('\n').map_or(src.len(), |i| start + i);
        let text = src[line_start..line_end].trim_end_matches('\r');

        let mut caret = String::new();
        for &b in &bytes[line_start..start] {
            if b == b'\t' {
                caret.push('\t');
            } else if b & 0xC0 != 0x80 {
                caret.push(' ');
            }
        }

        let end = (span.end as usize).min(line_end).max(start);
        let width = bytes[start..end].iter().filter(|&&b| b & 0xC0 != 0x80).count().max(1);
        caret.push_str(&"^".repeat(width));

        format!("{}:{}:{}: error: {}\n{}\n{}", file, line, col, msg, text, caret)
    }
}

pub fn line_col(src: &str, offset: u32) -> (u32, u32) {
    let mut line = 1;
    let mut col = 1;
    let end = (offset as usize).min(src.len());
    
    for &b in &src.as_bytes()[..end] {
        if b == b'\n' {
            line += 1;
            col = 1;
        } else if b & 0xC0 != 0x80 {
            col += 1;
        }
    }
    
    (line, col)
}