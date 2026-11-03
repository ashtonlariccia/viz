#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Span {
    pub start: u32,
    pub end: u32,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Token {
    pub token_kind: TokenKind,
    pub span: Span,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum TokenKind {
    Dec,
    Struct,
    Fn,
    Var,
    Exec,
    Load,
    Mut,
    Pk,
    Return,
    If,
    Else,
    While,
    For,
    Continue,
    Break,
    Unterm,
    Undef,

    Prim(PrimType),

    LowIdent,
    CapIdent,
    AllcapsIdent,

    IntLit(u64), // a literal '5' inline
    StrLit,      // a literal 'x'
    Builtin,

    Star,
    Amper,
    Exclaim,
    Minus,
    Plus,
    Fslash,
    Arrow,
    Dollar,
    Pcent,
    DoubleAmper,
    Equals,
    DoubleEquals,
    Notequal,
    Lthan,
    Gthan,
    LthanEqu,
    GthanEqu,
    DoublePipe,
    PlusEq,
    MinusEq,
    StarEq,
    FslashEq,
    PcentEq,
    Comma,
    Semicolon,
    Dot,
    Colon,

    Lparen,
    Rparen,
    Lbrack,
    Rbrack,
    Lbrace,
    Rbrace,

    Eof,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum PrimType {
    I8,
    I16,
    I32,
    I64,
    U8,
    U16,
    U32,
    U64,
    Usize,
    Isize,
}
